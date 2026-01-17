import Foundation

class DiskCacheProvider {
    private let logger: ILogger
    private let fileManager: ValdiFileManager
    private let projectConfig: ValdiProjectConfig
    private let disableDiskCache: Bool

    init(logger: ILogger, fileManager: ValdiFileManager, projectConfig: ValdiProjectConfig, disableDiskCache: Bool) {
        self.logger = logger
        self.fileManager = fileManager
        self.projectConfig = projectConfig
        self.disableDiskCache = disableDiskCache
    }

    func isEnabled() -> Bool {
        return !disableDiskCache
    }

    func newCache(cacheName: String, outputExtension: String, metadata: [String: String]) -> DiskCache? {
        if disableDiskCache {
            return nil
        } else {
            return DiskCacheImpl(logger: logger,
                                 fileManager: fileManager,
                                 projectConfig: projectConfig,
                                 cacheName: cacheName,
                                 outputExtension: outputExtension,
                                 metadata: metadata)
        }
    }
}
