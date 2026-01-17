//
//  IdentifyItemsProcessor.swift
//  Compiler
//
//  Created by Simon Corsin on 7/24/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

import Foundation

// [.unknown] -> [.rawDocument, .typeScript, .javaScript, .css, .rawResource, .unknown]
class IdentifyItemsProcessor: CompilationProcessor {

    var description: String {
        return "Identifying items"
    }

    private let logger: ILogger
    private let ignoredFiles: [NSRegularExpression]

    init(logger: ILogger, ignoredFiles: [NSRegularExpression]) {
        self.logger = logger
        self.ignoredFiles = ignoredFiles
    }

    private func shouldIgnore(inputURL: URL) -> Bool {
        let filename = inputURL.lastPathComponent
        return filename.matches(anyRegex: ignoredFiles)
    }    

    private func findKind(inputURL: URL, bundle: CompilationItem.BundleInfo, relativeProjectPath: String) -> CompilationItem.Kind {
        let filename = inputURL.lastPathComponent

        if filename == Files.classMapping {
            return .classMapping(.url(inputURL))
        } else if filename == Files.compilationMetadata {
            return .compilationMetadata(.url(inputURL))
        } else if filename.hasPrefix(Files.stringsJSONPrefix) && filename.hasSuffix(Files.stringsJSONSuffix) {
            return .stringsJSON(.url(inputURL))
        } else if filename == Files.moduleYaml {
            return .moduleYaml(.url(inputURL))
        } else if filename == Files.buildFile {
            return .buildFile(.url(inputURL))
        } else if filename == Files.ids {
            return .ids(.url(inputURL))
        } else if filename == Files.sqlTypesYaml {
            return .sql(.url(inputURL))
        } else if filename == Files.sqlManifestYaml {
            return .sqlManifest(.url(inputURL))
        }

        switch inputURL.pathExtension {
        case FileExtensions.vue:
            return .rawDocument(.url(inputURL))
        case FileExtensions.typescript, FileExtensions.typescriptXml:
            return .typeScript(.url(inputURL), inputURL)
        case FileExtensions.javascript:
            return .javaScript(JavaScriptFile(file: .url(inputURL), relativePath: relativeProjectPath))
        case FileExtensions.sass, FileExtensions.css:
            return .css(.url(inputURL))
        case FileExtensions.kotlin:
            return .kotlin(.url(inputURL), inputURL)
        case FileExtensions.protoDecl:
            return .resourceDocument(DocumentResource(outputFilename: inputURL.lastPathComponent, file: .url(inputURL)))
        case FileExtensions.json:
            return .resourceDocument(DocumentResource(outputFilename: bundle.itemPath(forItemURL: inputURL), file: .url(inputURL)))
        case FileExtensions.bin:
            return .resourceDocument(DocumentResource(outputFilename: bundle.itemPath(forItemURL: inputURL), file: .url(inputURL)))
        case FileExtensions.downloadableArtifacts:
            return .downloadableArtifactSignatures(DownloadableArtifactSignatures(key: inputURL.deletingPathExtension().lastPathComponent, file: .url(inputURL)))
        case let ext where FileExtensions.sql.contains(ext):
            return .sql(.url(inputURL))
        case let ext where FileExtensions.images.contains(ext) || FileExtensions.fonts.contains(ext):
            return .rawResource(.url(inputURL))
        default:
            return .unknown(.url(inputURL))
        }
    }

    func process(items: CompilationItems) throws -> CompilationItems {
        return items.selectAll().transformEachConcurrently { (item) -> [CompilationItem] in
            if case .unknown(let file) = item.kind, case .url(let url) = file {
                if shouldIgnore(inputURL: url) {
                    logger.debug("-- Ignoring file: \(url.lastPathComponent)")
                    return []
                }

                let newKind = findKind(inputURL: url, bundle: item.bundleInfo, relativeProjectPath: item.relativeProjectPath)
                return [item.with(newKind: newKind)]
            }
            return [item]
        }
    }

}
