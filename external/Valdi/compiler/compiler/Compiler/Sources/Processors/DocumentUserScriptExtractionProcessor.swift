//
//  DocumentUserScriptExtraction.swift
//  Compiler
//
//  Created by saniul on 16/06/2019.
//

import Foundation

final class DocumentUserScriptExtractionProcessor: CompilationProcessor {
    let userScriptManager: UserScriptManager

    private let logger: ILogger
    private let fileManager: ValdiFileManager
    private let projectConfig: ValdiProjectConfig

    init(logger: ILogger, fileManager: ValdiFileManager, userScriptManager: UserScriptManager, projectConfig: ValdiProjectConfig) {
        self.logger = logger
        self.fileManager = fileManager
        self.userScriptManager = userScriptManager
        self.projectConfig = projectConfig
    }

    var description: String {
        return "Extracting user scripts from Documents"
    }

    func process(items: CompilationItems) throws -> CompilationItems {
        return try items.select {
            if case .parsedDocument(let parsedDocument) = $0.kind {
                return parsedDocument
            } else {
                return nil
            }
            }.transformEachConcurrently(compile)
    }

    private func compile(selectedItem: SelectedItem<ValdiRawDocument>) throws -> [CompilationItem] {
        return compile(item: selectedItem.item, rawDocument: selectedItem.data)
    }

    private func compile(item: CompilationItem, rawDocument: ValdiRawDocument) -> [CompilationItem] {
        let inputURL = item.sourceURL
        logger.debug("-- Extracting user scripts from Document \(inputURL.lastPathComponent)")
        do {
            var out: [CompilationItem] = [item]

            if let result = try userScriptManager.processUserScript(documentItem: item, rawDocument: rawDocument),
                let emitUserScriptItem = result.emitScriptCompilationItem {
                // Store the extracted file in the generatedTsDirectory so that we get autocompletion in VSCode
                let definitionURL = projectConfig.generatedTsDirectoryURL.appendingPathComponent(emitUserScriptItem.relativeProjectPath)
                try fileManager.save(file: result.userScript.content, to: definitionURL)

                out.append(emitUserScriptItem)
            }

            return out
        } catch let error {
            logger.error("Failed to extract user scripts from Document \(inputURL): \(error.legibleLocalizedDescription)")
            return [item.with(error: error)]
        }
    }
}
