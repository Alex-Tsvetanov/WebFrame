//
//  SaveFilesProcessor.swift
//  Compiler
//
//  Created by Simon Corsin on 7/24/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

import Foundation

// [.finalFile] -> []
class SaveFilesProcessor: CompilationProcessor {

    private let logger: ILogger
    private let fileManager: ValdiFileManager
    private let projectConfig: ValdiProjectConfig

    private var processedFiles = Set<URL>()
    private let modulesFilter: ModulesFilter?
    private let removeOrphanFiles: Bool
    private let hotReloadingEnabled: Bool

    var description: String {
        return "Saving files"
    }

    init(logger: ILogger, fileManager: ValdiFileManager, projectConfig: ValdiProjectConfig, modulesFilter: ModulesFilter?,
        removeOrphanFiles: Bool, hotReloadingEnabled: Bool) {
        self.logger = logger
        self.fileManager = fileManager
        self.projectConfig = projectConfig
        self.modulesFilter = modulesFilter
        self.removeOrphanFiles = removeOrphanFiles
        self.hotReloadingEnabled = hotReloadingEnabled
    }

    private func saveFile(file: File, to outUrl: URL) throws {
        try fileManager.save(file: file, to: outUrl, options: [.overwriteOnlyIfDifferent])
    }

    private func doSave(item: SelectedItem<FinalFile>) -> [CompilationItem] {
        do {
            try saveFile(file: item.data.file, to: item.data.outputURL)
            logger.verbose("Saving final file to \(item.data.outputURL.path)")
            return []
        } catch let error {
            let errorMessage = "Failed to save file to \(item.data.outputURL.path): \(error.legibleLocalizedDescription)"
            logger.error(errorMessage)
            return [item.item.with(error: error)]
        }
    }

    private func saveProcessedFiles(processedFiles: [SelectedItem<FinalFile>]) {
        for processedFile in processedFiles {
            self.processedFiles.insert(processedFile.data.outputURL.standardized)
        }
    }

    private func doRemoveOrphanFiles(rootDirectoryURL: URL) {
        let allFiles: [URL]
        do {
            let filesFinder = ValdiFilesFinder(url: rootDirectoryURL)
            allFiles = try filesFinder.allFiles()
        } catch let error {
            logger.error("Failed to remove : \(error.legibleLocalizedDescription)")
            return
        }

        for file in allFiles {
            guard !processedFiles.contains(file) else {
                continue
            }

            // orphaned strings files will get removed in the next localization service pass
            guard file.lastPathComponent.starts(with: "valdi-strings-") == false else {
                continue
            }

            do {
                try FileManager.default.removeItem(at: file)
                logger.info("Removed orphan file at \(file.path)")
            } catch let error {
                logger.error("Failed to remove orphan file at \(file.path): \(error.legibleLocalizedDescription)")
            }
        }
    }

    func process(items: CompilationItems) throws -> CompilationItems {

        let finalFiles = items.select { item -> FinalFile? in
            switch item.kind {
            case let .finalFile(finalFile):
                if (hotReloadingEnabled && finalFile.platform != Platform.web) {
                    // Only web files get written to disk when hotreloading
                    return nil
                }

                if let modulesFilter = modulesFilter, !modulesFilter.shouldIncludeInOutput(bundleInfo: item.bundleInfo) {
                    return nil
                }
                
                if let platform = finalFile.platform {
                    if let androidIgnoredFiles = projectConfig.androidOutput?.ignoredFiles, platform == .android, finalFile.outputURL.lastPathComponent.matches(anyRegex: androidIgnoredFiles) {
                        return nil
                    }
                    if let iosIgnoredFiles = projectConfig.iosOutput?.ignoredFiles, platform == .ios, finalFile.outputURL.lastPathComponent.matches(anyRegex: iosIgnoredFiles) {
                        return nil
                    }
                }

                return finalFile
            default:
                return nil
            }
        }

        let items = finalFiles.transformEachConcurrently(doSave)
        try removeOrphanFilesIfNeeded(finalFiles: finalFiles)
        return items
    }

    private func getResolvedBaseURL(unresolvedPath: UnresolvedPath?) throws -> URL? {
        guard let unresolvedPath = unresolvedPath else {
            return nil
        }
        guard unresolvedPath.components.count == 2 else {
            throw CompilerError("debugPath or releasePath should have 2 components, found \(unresolvedPath.components.count) ")
        }

        return try UnresolvedPath(baseURL: unresolvedPath.baseURL, components: [unresolvedPath.components[0]], variables: unresolvedPath.variables).resolve()
    }

    private func removeOrphanFilesIfNeeded(finalFiles: SelectedCompilationItems<SelectedItem<FinalFile>>) throws {
        guard removeOrphanFiles else {
            return
        }

        logger.info("Removing orphan files")
        finalFiles.processAll(saveProcessedFiles)

        var uniqueModuleRootDirectoryURLs = Set<URL>()

        finalFiles.groupBy { $0.item.bundleInfo }.processAll { allBundles in
            allBundles.forEach { byBundle in
                let bundleInfo = byBundle.key

                let urlsForThisModule = [
                    bundleInfo.androidDebugOutputDirectories?.baseURL,
                    bundleInfo.androidExpectedReleaseOutputDirectories?.baseURL,
                    bundleInfo.iosDebugOutputDirectories?.baseURL,
                    bundleInfo.iosExpectedReleaseOutputDirectories?.baseURL,
                    bundleInfo.webDebugOutputDirectories?.baseURL,
                    bundleInfo.webExpectedReleaseOutputDirectories?.baseURL,
                ].compactMap { $0 }
                uniqueModuleRootDirectoryURLs.formUnion(urlsForThisModule)
            }
        }
        let moduleRootDirectoryURLs = Array(uniqueModuleRootDirectoryURLs)
        moduleRootDirectoryURLs.parallelProcess(doRemoveOrphanFiles)

        let rootDirectoryURLs: [URL] = [
            try getResolvedBaseURL(unresolvedPath: projectConfig.iosOutput?.debugPath),
            try getResolvedBaseURL(unresolvedPath: projectConfig.iosOutput?.releasePath),
            try getResolvedBaseURL(unresolvedPath: projectConfig.androidOutput?.debugPath),
            try getResolvedBaseURL(unresolvedPath: projectConfig.androidOutput?.releasePath),
            try getResolvedBaseURL(unresolvedPath: projectConfig.webOutput?.debugPath),
            try getResolvedBaseURL(unresolvedPath: projectConfig.webOutput?.releasePath),
        ].compactMap { $0 }

        deleteUnknownDirectories(rootDirectoryURLs: rootDirectoryURLs,
                                 moduleRootDirectoryURLs: moduleRootDirectoryURLs)
    }

    // directories that don't match any processed module
    private func deleteUnknownDirectories(rootDirectoryURLs: [URL], moduleRootDirectoryURLs: [URL]) {
        for rootDir in rootDirectoryURLs {
            let results = (try? FileManager.default.contentsOfDirectory(at: rootDir, includingPropertiesForKeys: [.isDirectoryKey], options: [])) ?? []
            let directoriesOnly = results.filter { (url) -> Bool in
                do {
                    let resourceValues = try url.resourceValues(forKeys: [.isDirectoryKey])
                    return resourceValues.isDirectory ?? false
                } catch { return false }
            }

            let unknownDirectories = Set(directoriesOnly)
                .map { $0.standardizedFileURL }
                .filter { possiblyUnknownDirectory in
                return possiblyUnknownDirectory.pathExtension.isEmpty // ignore the .xcodeproj, we just want to clean up actual directories
                    && !moduleRootDirectoryURLs.contains(where: { $0.absoluteString.hasPrefix(possiblyUnknownDirectory.absoluteString) })
                    && projectConfig.directoriesToKeepRegexes.allSatisfy({ !possiblyUnknownDirectory.relativeString.matches(regex: $0) })
            }

            Array(unknownDirectories).parallelProcess { unknownDirectory in
                do {
                    try FileManager.default.removeItem(at: unknownDirectory)
                    logger.warn("Removed unknown directory at \(unknownDirectory.path)")
                } catch let error {
                    logger.error("Failed to unknown directory at \(unknownDirectory.path): \(error.legibleLocalizedDescription)")
                }
            }
        }
    }

}
