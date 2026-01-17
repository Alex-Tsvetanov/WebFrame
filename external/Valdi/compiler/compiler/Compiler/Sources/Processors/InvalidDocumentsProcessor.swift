//
//  InvalidDocumentsProcessor.swift
//  Compiler
//
//  Created by Simon Corsin on 7/24/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

import Foundation

// [.invalidDocument] -> [.resourceDocument]
class InvalidDocumentsProcessor: CompilationProcessor {

    var description: String {
        return "Handling document errors"
    }

    private let logger: ILogger
    private let generateErrorDocuments: Bool
    private let compilationPipeline: [CompilationProcessor]
    private let projectConfig: ValdiProjectConfig

    init(logger: ILogger, generateErrorDocuments: Bool, compilationPipeline: [CompilationProcessor], projectConfig: ValdiProjectConfig) {
        self.logger = logger
        self.generateErrorDocuments = generateErrorDocuments
        self.compilationPipeline = compilationPipeline
        self.projectConfig = projectConfig
    }

    private func makeItems(forKotlinError error: Error, item: CompilationItem) -> [CompilationItem] {
        // TODO(simon): Implements
        // Need some kind of way to know which components have failed
        return []
    }

    private func makeItems(forError error: Error, message: String, item: CompilationItem) -> [CompilationItem] {
        let relativeURL = URL(string: item.relativeProjectPath)!

        guard relativeURL.pathExtension != FileExtensions.vue else {
            let relativeURLWithExtension = relativeURL.appendingPathExtension(FileExtensions.typescript)
            // Case for vue template which failed compilation before we emitted the item.
            // We generate a new item as if the template compiled succesfully with the error
            // script.
            let generatedSourceURL = TypeScriptCompilerManager.generatedUrl(url: relativeURLWithExtension)
            let generatedItem = CompilationItem(sourceURL: generatedSourceURL,
                                                relativeProjectPath: relativeURLWithExtension.path,
                                                kind: .error(error, originalItemKind: item.kind),
                                                bundleInfo: item.bundleInfo,
                                                platform: item.platform,
                                                outputTarget: item.outputTarget)
            return makeItems(forError: error, message: message, item: generatedItem)
        }

        //TODO(3521): update to valdi_core
        let errorDocument = """
        let errorClass;
        try {
          errorClass = require('valdi_core/src/utils/CompilerError').CompilerError;
        } catch (err) {
          errorClass = Error;
        }
        throw new errorClass('\(message.jsonEscaped)');
        """

        do {
            let data = try errorDocument.utf8Data()
            let relativePath = relativeURL.deletingPathExtension().appendingPathExtension(FileExtensions.javascript).path
            return [item.with(newKind: .javaScript(JavaScriptFile(file: .data(data), relativePath: relativePath)))]
        } catch let error {
            logger.error("Failed to generate Error document: \(error.legibleLocalizedDescription)")
            return []
        }
    }

    func process(items: CompilationItems) throws -> CompilationItems {
        var out = [CompilationItem]()
        var failuresCount = 0

        func handle(error: Error, messagePrefix: String, item: CompilationItem) {
            let message = "\(messagePrefix): \(error.legibleLocalizedDescription)"

            if generateErrorDocuments {
                out += makeItems(forError: error, message: message, item: item)
            } else {
                failuresCount += 1
            }
        }

        for item in items.allItems {
            if case .error(let error, let sourceItem) = item.kind {
                switch sourceItem {
                case .rawDocument, .document, .parsedDocument:
                    handle(error: error, messagePrefix: "Failed to process document from \(item.relativeProjectPath)", item: item)
                case .typeScript:
                    handle(error: error, messagePrefix: "Failed to compile TypeScript from \(item.relativeProjectPath)", item: item)
                case .stringsJSON:
                    let messagePrefix = "Failed to compile strings from \(item.relativeProjectPath): \(error.legibleLocalizedDescription)"
                    let jsOutputPath = TypeScriptStringsModuleGenerator.outputPath(bundleName: item.bundleInfo.name, ext: FileExtensions.javascript)
                    let jsOutputURL = item.bundleInfo.absoluteURL(forRelativeProjectPath: jsOutputPath)
                    let item = CompilationItem(sourceURL: jsOutputURL, relativeProjectPath: nil, kind: .error(error, originalItemKind: sourceItem), bundleInfo: item.bundleInfo, platform: item.platform, outputTarget: item.outputTarget)
                    handle(error: error, messagePrefix: messagePrefix, item: item)
                case .kotlin:
                    handle(error: error, messagePrefix: "Failed to compile Kotlin from \(item.relativeProjectPath)", item: item)
                default:
                    break
                }
            } else {
                out.append(item)
            }
        }

        if failuresCount > 0 {
            throw CompilerError("Got \(failuresCount) errors.")
        }

        return CompilationItems(compileSequence: items.compileSequence, items: out)
    }

}
