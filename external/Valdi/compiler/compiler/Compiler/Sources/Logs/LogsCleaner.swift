//
//  LogsCleaner.swift
//  
//
//  Created by saniul on 03/12/2021.
//

import Foundation

/// Cleans up outdated logs files
final class LogsCleaner {
    private let logger: ILogger
    private let logsDirectoryURL: URL

    init(logger: ILogger, logsDirectoryURL: URL) {
        self.logger = logger
        self.logsDirectoryURL = logsDirectoryURL
    }

    public func cleanLogsFilesIfNeeded() {
        do {
            try _cleanLogsFilesIfNeeded()
        } catch {
            logger.error("Failed to clean logs files from \(logsDirectoryURL): \(error.legibleLocalizedDescription)")
        }
    }

    private func _listAllLogsFiles() throws -> [LogsFileProtocol] {
        let fileURLs = try FileManager.default.contentsOfDirectory(at: logsDirectoryURL,
                                                includingPropertiesForKeys: [.contentModificationDateKey,
                                                     .fileSizeKey],
                                                options: [])
        let logsFiles = fileURLs
            .filter({ $0.pathExtension == "log" })
            .map(LogsFileHandle.init)
        return logsFiles
    }

    private func _shouldDeleteLogsFile(_ file: LogsFileProtocol) -> Bool {
        guard let modificationDate = file.modificationDate else {
            return false
        }

        // delete logs that have not been modified in over 30 days
        let isTooOld = abs(modificationDate.timeIntervalSinceNow) > 60 * 60 * 24 * 30
        return isTooOld
    }

    private func _cleanLogsFilesIfNeeded() throws {
        let logsFiles = try _listAllLogsFiles()
        for logsFile in logsFiles {
            if _shouldDeleteLogsFile(logsFile) {
                logger.info("Deleting outdated log file at \(logsFile.fileURL) (\(logsFile.fileSizeString))...")
                do {
                    try FileManager.default.removeItem(at: logsFile.fileURL)
                } catch {
                    logger.error("Failed to delete logs file at \(logsFile.fileURL): \(error.legibleLocalizedDescription)")
                }
            }
        }
    }
}
