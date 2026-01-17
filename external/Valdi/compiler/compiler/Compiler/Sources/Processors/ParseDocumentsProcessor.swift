//
//  ParseDocumentsProcessor.swift
//  Compiler
//
//  Created by Simon Corsin on 7/24/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

import Foundation

// [.document] -> [.parsedDocument, .invalidDocument]
class ParseDocumentsProcessor: CompilationProcessor {

    var description: String {
        return "Parsing Documents"
    }

    private let logger: ILogger
    private let globalIosImportPrefix: String?

    init(logger: ILogger, globalIosImportPrefix: String?) {
        self.logger = logger
        self.globalIosImportPrefix = globalIosImportPrefix
    }

    private func mergeStyles(styles: [ValdiStyleSheet]) throws -> String {
        var out = ""

        for style in styles {
            if let content = style.content {
                out += content
            }
            if let _ = style.source {
                throw CompilerError("src= is not implemented, please use @import instead")
            }
        }

        return out.trimmed
    }

    private func parse(selectedItem: SelectedItem<File>) -> [CompilationItem] {
        let sourceURL = selectedItem.item.sourceURL
        do {
            logger.debug("-- Parsing \(sourceURL.lastPathComponent)")
            let fileContent = try selectedItem.data.readString()
            let parsedDocument = try ValdiDocumentParser.parse(logger: logger,
                                                                  content: fileContent,
                                                                  iosImportPrefix: selectedItem.item.bundleInfo.iosModuleName)

            let cssContent = try mergeStyles(styles: parsedDocument.styles)

            return [
                selectedItem.item.with(newKind: .parsedDocument(parsedDocument)),
                selectedItem.item.with(newKind: .css(.data(try cssContent.utf8Data())))
            ]
        } catch let error {
            logger.error("Failed to parse \(sourceURL.path): \(error.legibleLocalizedDescription)")
            return [selectedItem.item.with(error: error)]
        }
    }

    func process(items: CompilationItems) throws -> CompilationItems {
        return items.select { (item) -> File? in
            if case .rawDocument(let file) = item.kind {
                return file
            }
            return nil
        }.transformEachConcurrently(parse)
    }

}
