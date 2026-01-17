// Copyright © 2024 Snap, Inc. All rights reserved.

import Foundation

open class FileHandleReader {
    var onDidReceiveData: ((_ reader: FileHandleReader) -> Void)?

    var content: Data {
        get {
            if dispatchQueue == nil {
                readToEnd()
            }
            return buffer
        }
        set(newValue) {
            buffer = newValue
        }
    }
    
    var contentAsString: String {
        get {
            return String(data: content, encoding: .utf8) ?? ""
        }
        set {
            content = newValue.data(using: .utf8) ?? Data()
        }
    }

    private var buffer = Data()

    private let dispatchQueue: DispatchQueue?
    private let fileHandle: FileHandle
    private var channel: DispatchIO?
    private var onDonePromises: [Promise<Void>]?

    init(fileHandle: FileHandle, dispatchQueue: DispatchQueue?) {
        self.dispatchQueue = dispatchQueue
        self.fileHandle = fileHandle

        if let dispatchQueue = dispatchQueue {
            self.channel = DispatchIO(type: .stream, fileDescriptor: fileHandle.fileDescriptor, queue: dispatchQueue) { (_) in

            }
            channel?.setLimit(lowWater: 1)
            scheduleRead(queue: dispatchQueue)
        }
    }

    deinit {
        close()
    }

    func trimContent(by offset: Int) {
        buffer.removeSubrange(0..<offset)
    }

    func readToEnd() {
        let data = fileHandle.readDataToEndOfFile()
        if !data.isEmpty {
            if buffer.isEmpty {
                buffer = data
            } else {
                buffer.append(data)
            }
        }
    }

    func waitUntilDone() throws {
        if let dispatchQueue {
            let promise = Promise<Void>()
            dispatchQueue.sync {
                if channel == nil {
                    promise.resolve(data: ())
                } else {
                    if self.onDonePromises == nil {
                        self.onDonePromises = []
                    }
                    self.onDonePromises?.append(promise)
                }
            }

            try promise.waitForData()
        } else {
            readToEnd()
        }
    }

    private func close() {
        if let channel = self.channel {
            self.channel = nil
            channel.close()
        }
    }

    private func onDone() {
        close()
        if let onDonePromises = self.onDonePromises {
            self.onDonePromises = nil
            onDonePromises.forEach { $0.resolve(data: ()) }
        }
    }

    private func scheduleRead(queue: DispatchQueue) {
        channel?.read(offset: 0, length: Int.max, queue: queue, ioHandler: { [weak self] (done, data, _) in
            guard let self else { return }
            if let data = data {
                self.didReceive(data: data)
            }

            if done {
                self.onDone()
            } else {
                self.scheduleRead(queue: queue)
            }
        })
    }

    private func didReceive(data: DispatchData) {
        if !data.isEmpty {
            for region in data.regions {
                region.withUnsafeBytes { buffer in
                    self.buffer.append(buffer.bindMemory(to: UInt8.self).baseAddress!, count: region.count)
                }
            }
            self.onDidReceiveData?(self)
        }
    }

}
