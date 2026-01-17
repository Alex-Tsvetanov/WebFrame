//
//  LogsWriter.swift
//  Compiler
//
//  Created by Simon Corsin on 2/19/19.
//  Copyright © 2019 Snap Inc. All rights reserved.
//

import Foundation

private let startEndSessionDelimiter = "---------------"

final class LogsWriter {

    let outputURL: URL

    private let logger: ILogger
    private let outputStream: OutputStream
    private let queue = DispatchQueue(label: "com.snap.valdi.LogsWriter")

    init(logger: ILogger, fileManager: ValdiFileManager, outputURL: URL) throws {
        self.logger = logger
        self.outputURL = outputURL

        let directoryURL = outputURL.deletingLastPathComponent()
        try fileManager.createDirectory(at: directoryURL)

        if !FileManager.default.fileExists(atPath: outputURL.path) {
            FileManager.default.createFile(atPath: outputURL.path, contents: nil, attributes: nil)
        }

        let logsRotator = LogsRotator(logger: logger, logsDirectoryURL: directoryURL)
        logsRotator.rotateLogsFileIfNeeded(LogsFileHandle(fileURL: outputURL))

        guard let outputStream = OutputStream(toFileAtPath: outputURL.path, append: true) else {
            throw CompilerError("Unable to open output stream to write logs at \(outputURL.path)")
        }
        outputStream.open()

        self.outputStream = outputStream

        queue.async {
            self.doWrite(formattedLog: "\n\(startEndSessionDelimiter) Session started at \(Date().description) \(startEndSessionDelimiter)\n")
        }
    }

    func close() {

        queue.async {
            self.doWrite(formattedLog: "\n\(startEndSessionDelimiter) Session ended at \(Date().description) \(startEndSessionDelimiter)\n")
            self.outputStream.close()
        }
    }

    private func doWrite(formattedLog: String) {
        do {
            let data = try formattedLog.utf8Data()
            data.withUnsafePointer { bufferStart in
                _ = self.outputStream.write(bufferStart, maxLength: data.count)
            }
        } catch let error {
            logger.error("Unable to convert log to utf8: \(error.legibleLocalizedDescription)")
        }
    }

    func write(log: String) {
        let date = Date()
        queue.async {
            let formattedLog = "\(date.description) \(log)\n"
            self.doWrite(formattedLog: formattedLog)
        }
    }

}
