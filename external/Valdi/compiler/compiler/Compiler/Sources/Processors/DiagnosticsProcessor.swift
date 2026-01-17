//
//  DiagnosticsProcessor.swift
//  Compiler
//
//  Created by saniul on 19/07/2019.
//

import Foundation

// Collates all the diagnostics generated for a given source file together.
// Outputs a File that can then be emitted in the final output.
//
// At the moment we only have GeneratedTypeDescription diagnostics,
// but if we had more, this Processor is where we would collate all the diagnostics that are
// relevant to a given source file, and put them in a common structure.
class DiagnosticsProcessor: CompilationProcessor {

    let projectConfig: ValdiProjectConfig

    var description: String {
        return "Summarizing Diagnostics by Source File"
    }

    init(projectConfig: ValdiProjectConfig) {
        self.projectConfig = projectConfig
    }

    func process(items: CompilationItems) throws -> CompilationItems {
        guard projectConfig.shouldEmitDiagnostics else {
            return items
        }

        return items.select { (item) -> GeneratedTypeDescription? in
            if case let .generatedTypeDescription(generatedTypeDescription) = item.kind {
                return generatedTypeDescription
            }
            return nil
            }.groupBy { selectedItem -> String in
                selectedItem.item.relativeProjectPath
            }.transformEachConcurrently { groupedItems -> CompilationItem in
                let relativeSourceFilePath = groupedItems.key
                let descriptions = groupedItems.items.map { $0.data }.sorted { $0.valueToSortBy < $1.valueToSortBy }
                let summary = GeneratedTypesSummary(sourceFilePath: relativeSourceFilePath,
                                                    generatedTypes: descriptions)
                let anyItem = groupedItems.items[0]

                let file = summary.toFile()

                return anyItem.item.with(newKind: .diagnosticsFile(file),
                                         newPlatform: .none)
            }
    }

}
