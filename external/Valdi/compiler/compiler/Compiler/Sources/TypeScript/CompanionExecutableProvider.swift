// Copyright © 2024 Snap, Inc. All rights reserved.

import Foundation

class CompanionExecutableProvider {

    private let logger: ILogger
    private var lastCompanionExecutable: CompanionExecutable? = nil
    private let lock = DispatchSemaphore.newLock()

    init(logger: ILogger) {
        self.logger = logger
    }

    func getCompanionExecutable(compilerCompanionBinaryPath: String,
                                nodeOptions: [String],
                                logsOutputPath: String?,
                                compilerCacheURL: URL?,
                                isBazelBuild: Bool) -> CompanionExecutable {
        lock.lock {
            if let lastCompanionExecutable,
                lastCompanionExecutable.compilerCompanionBinaryPath == compilerCompanionBinaryPath,
               lastCompanionExecutable.nodeOptions == nodeOptions,
               lastCompanionExecutable.logsOutputPath == logsOutputPath,
               lastCompanionExecutable.compilerCacheURL == compilerCacheURL,
               lastCompanionExecutable.isBazelBuild == isBazelBuild,
               !lastCompanionExecutable.processExited {
                return lastCompanionExecutable
            }

            let companionExecutable = CompanionExecutable(logger: self.logger,
                                                          compilerCompanionBinaryPath: compilerCompanionBinaryPath,
                                                          nodeOptions: nodeOptions,
                                                          logsOutputPath: logsOutputPath,
                                                          compilerCacheURL: compilerCacheURL,
                                                          isBazelBuild: isBazelBuild)
            self.lastCompanionExecutable = companionExecutable

            return companionExecutable
        }
    }

    func getCompanionExecutable(compilerConfig: CompilerConfig,
                                projectConfig: ValdiProjectConfig,
                                logsOutputPath: String?,
                                disableTsCompilerCache: Bool,
                                diskCacheProvider: DiskCacheProvider,
                                isBazelBuild: Bool) throws -> CompanionExecutable {
        let compilerCompanionBinaryPath: String
        var nodeOptions = [String]()
        let compilerCacheURL: URL?
        if let directCompanionPath = compilerConfig.directCompanionPath {
            compilerCompanionBinaryPath = directCompanionPath
        } else {
            guard let companionBinaryURL = projectConfig.compilerCompanionBinaryURL else {
                throw CompilerError("Need compiler_companion_bin set in the config.yaml")
            }
            compilerCompanionBinaryPath = companionBinaryURL.path
            nodeOptions.append("--max-old-space-size=4096")
            if projectConfig.shouldDebugCompilerCompanion {
                nodeOptions.append("--inspect")
            }
        }

        if !disableTsCompilerCache, let diskCache = diskCacheProvider.newCache(cacheName: "companion", outputExtension: "", metadata: [:]) {
            compilerCacheURL = diskCache.getURL().absoluteURL
        } else {
            compilerCacheURL = nil
        }

        return getCompanionExecutable(
            compilerCompanionBinaryPath: compilerCompanionBinaryPath,
            nodeOptions: nodeOptions,
            logsOutputPath: logsOutputPath,
            compilerCacheURL: compilerCacheURL,
            isBazelBuild: isBazelBuild)
    }

}
