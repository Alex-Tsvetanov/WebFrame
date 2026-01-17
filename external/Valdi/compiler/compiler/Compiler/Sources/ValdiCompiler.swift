//
//  ValdiCompiler.swift
//  Compiler
//
//  Created by Simon Corsin on 4/6/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

import Foundation
import Yams

final class ValdiCompiler {

    private struct NewFileData {
        let bundleInfo: CompilationItem.BundleInfo?
        let relativeProjectPath: String?
        let content: String?
    }

    private let logger: ILogger
    private let pipeline: CompilationPipeline
    private let projectConfig: ValdiProjectConfig

    private var newFiles = [URL: NewFileData]()

    private let bundleManager: BundleManager
    private var currentSequence = 0

    init(logger: ILogger, pipeline: CompilationPipeline, projectConfig: ValdiProjectConfig, bundleManager: BundleManager) {
        self.logger = logger
        self.pipeline = pipeline
        self.projectConfig = projectConfig
        self.bundleManager = bundleManager
    }

    func addFiles(fromInputList inputList: CompilerFileInputList, baseUrl: URL) throws {
        for entry in inputList.entries {
            let bundleInfo = try bundleManager.getBundleInfo(forName: entry.moduleName)
            for file in entry.files {
                let fileURL = baseUrl.resolving(path: file.file).standardizedFileURL.absoluteURL
                addFile(file: fileURL, bundleInfo: bundleInfo, relativeProjectPath: file.relativeProjectPath, content: file.content)
            }

            if let autoDiscover = entry.autoDiscover, autoDiscover {
                let finder = ValdiFilesFinder(url: baseUrl.resolving(path: entry.modulePath))

                for fileURL in try finder.allFiles() {
                    addFile(file: try fileURL.resolvingSymlink(), bundleInfo: bundleInfo)
                }
            }
        }
    }

    func addFile(file: URL, bundleInfo: CompilationItem.BundleInfo? = nil, relativeProjectPath: String? = nil, content: String? = nil) {
        addFile(file: file, newFileData: NewFileData(bundleInfo: bundleInfo, relativeProjectPath: relativeProjectPath, content: content))
    }

    private func addFile(file: URL, newFileData: NewFileData) {
        newFiles[file] = newFileData
    }

    func compile() throws {
        self.currentSequence += 1
        let compileSequence = CompilationSequence(id: currentSequence)

        var newFileDataBySourceURL = [URL: NewFileData]()
        let allFiles = try newFiles.map { (sourceURL: URL, newFileData: NewFileData) -> CompilationItem in
            let resolvedBundleInfo = try (newFileData.bundleInfo ?? bundleManager.getBundleInfo(fileURL: sourceURL))
            let file: File
            if let content = newFileData.content {
                file = .string(content)
            } else {
                file = .url(sourceURL)
            }
            newFileDataBySourceURL[sourceURL] = newFileData

            return CompilationItem(sourceURL: sourceURL, relativeProjectPath: newFileData.relativeProjectPath, kind: .unknown(file), bundleInfo: resolvedBundleInfo, platform: nil, outputTarget: .all)
        }
        newFiles.removeAll()

        let result = try pipeline.process(items: CompilationItems(compileSequence: compileSequence, items: allFiles))

        if !result.warnings.isEmpty {
            logger.error("Warnings reported:")
            result.warnings.forEach {
                logger.error("-- \($0.tag): \($0.message)")
            }
        }

        if !result.failedItems.isEmpty {
            logger.error("Errors reported:")

            // Re-add the failed files into the newFiles
            for (sourceURL, failedItem) in result.failedItems {
                // Don't include the emitted files from the process
                guard let newFileData = newFileDataBySourceURL[sourceURL] else { continue }

                addFile(file: sourceURL, newFileData: newFileData)

                if let errorDescription = failedItem.error?.legibleLocalizedDescription {
                    logger.error("-- \(failedItem.relativeProjectPath): \(errorDescription)")
                }
            }
        }
    }
}
