import Foundation

/// Rotates logs files when they get too large.
///
/// Rotated logs files stop being regularly modified, so they will get cleaned up by LogsCleaner eventually.
public final class LogsRotator {
    private let logger: ILogger
    let logsDirectoryURL: URL

    init(logger: ILogger, logsDirectoryURL: URL) {
        self.logger = logger
        self.logsDirectoryURL = logsDirectoryURL
    }

    public func rotateLogsFileIfNeeded(_ file: LogsFileProtocol) {
        guard _shouldRotateLogsFile(file) else {
            return
        }

        _rotateLogsFile(file)
    }

    private func _shouldRotateLogsFile(_ file: LogsFileProtocol) -> Bool {
        guard file.fileSize > 0 else {
            return false
        }

        // rotate logs after they grow larger than 100 MB
        let isTooLarge = file.fileSize > 100_000_000
        return isTooLarge
    }

    private func _rotateLogsFile(_ file: LogsFileProtocol) {
        let fileName = file.fileURL.deletingPathExtension().lastPathComponent
        // insert current timestamp between the existing filename and file extension
        let newFileName = "\(fileName)-\(Int(Date().timeIntervalSince1970)).\(file.fileURL.pathExtension)"
        let newFileURL = logsDirectoryURL.appendingPathComponent(newFileName)

        logger.info("Rotating log file from \(file.fileURL) to \(newFileURL)...")
        // we just don't care about conflicts, unlikely that we'd end up rotating a log file with the same name (same client id) again.
        // It'd have to grow to be over the shouldRotateLogsFile instantly and the daemon service would have to disconnect and reconnect
        // in the same second.
        do {
            let attributes = try FileManager.default.attributesOfItem(atPath: file.fileURL.path)
            try FileManager.default.moveItem(at: file.fileURL, to: newFileURL)
            FileManager.default.createFile(atPath: file.fileURL.path, contents: nil, attributes: attributes)
        } catch {
            logger.warn("Failed to rotate logs file from \(file.fileURL) to \(newFileURL): \(error.legibleLocalizedDescription)")
        }

    }
}
