//
//  DispatchChannelTCPConnection.swift
//  Compiler
//
//  Created by saniul on 07/10/2019.
//

import Foundation

class DispatchChannelTCPConnection: TCPConnection {
    weak var delegate: TCPConnectionDelegate?
    var dataProcessor: TCPConnectionDataProcessor?

    let key: String
    let ioChannel: DispatchIO

    private let logger: ILogger
    private let sendQueue: DispatchQueue
    private let recvQueue: DispatchQueue
    private var startedListening: Bool = false
    private var buffer = Data()

    private enum Error: Swift.Error {
        case readEmptyData
    }

    init(logger: ILogger, key: String, ioChannel: DispatchIO, queueNamePrefix: String) {
        self.logger = logger
        self.key = key
        self.ioChannel = ioChannel
        sendQueue = DispatchQueue(label: "\(queueNamePrefix).\(key).send")
        recvQueue = DispatchQueue(label: "\(queueNamePrefix).\(key).recv")
    }

    deinit {
        ioChannel.close()
    }

    func startListening() {
        guard !startedListening else {
            return
        }

        startedListening = true
        recvQueue.async {
            self.receiveLoop()
        }
    }

    func close() {
        closeConnection(error: nil)
    }

    func send(data: Data) {
        sendQueue.async {
            self.logger.verbose("Writing \(data.count) bytes to channel \(self.key)")

            var retainedData: Data? = data
            let dispatchData: DispatchData = retainedData!.withUnsafeBytes { pointer in
                return DispatchData(bytes: pointer)
            }

            self.ioChannel.write(offset: 0, data: dispatchData, queue: self.sendQueue) { (done, _, errno) in
                defer { retainedData = nil }
                guard done else { return }

                if errno != 0 {
                    let error = NSError(domain: NSPOSIXErrorDomain, code: Int(errno), userInfo: nil)
                    self.logger.error("Error while writing to {\(self.key)}: \(error.legibleLocalizedDescription)")
                    self.closeConnection(error: error)
                }
            }
        }
    }

    private func receiveLoop(lengthNeeded: Int? = nil) {
        ioChannel.read(offset: 0, length: lengthNeeded ?? 1, queue: self.recvQueue) { [weak self] (done, dispatchData, errno) in
            guard let this = self else { return }

            guard errno == 0 else {
                let error = NSError(domain: NSPOSIXErrorDomain, code: Int(errno), userInfo: nil)
                this.closeConnection(error: error)
                return
            }

            guard let dispatchData = dispatchData, dispatchData.count > 0 else {
                this.closeConnection(error: Error.readEmptyData)
                return
            }

            this.buffer.append(Data(dispatchData))

            if done {
                let nextLengthNeeded = this.processBuffer(&this.buffer)

                this.receiveLoop(lengthNeeded: nextLengthNeeded)
            }
        }
    }

    private func closeConnection(error: Swift.Error?) {
        logger.debug("Closing reloader connection \(key) with error \(error?.legibleLocalizedDescription ?? "")")
        ioChannel.close()
        delegate?.connection(self, didDisconnectWithError: error)
    }
}
