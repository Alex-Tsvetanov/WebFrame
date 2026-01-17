//
//  ParseTypeScriptAnnotationsProcessor.swift
//  Compiler
//
//  Created by saniul on 13/06/2019.
//

import Foundation

struct DumpedTypeScriptSymbolsResult {
    let typeScriptItemAndSymbols: TypedScriptItemAndSymbols
}

final class DumpTypeScriptSymbolsProcessor: CompilationProcessor {

    var description: String {
        return "Checking TypeScript and Dumping Symbols"
    }

    private let logger: ILogger
    private let typeScriptCompilerManager: TypeScriptCompilerManager
    private let compilerConfig: CompilerConfig

    init(logger: ILogger, typeScriptCompilerManager: TypeScriptCompilerManager, compilerConfig: CompilerConfig) {
        self.logger = logger
        self.typeScriptCompilerManager = typeScriptCompilerManager
        self.compilerConfig = compilerConfig
    }

    func process(items: CompilationItems) throws -> CompilationItems {
        // open and check the user written TS files
        let userResult = try typeScriptCompilerManager.checkItems(compileSequence: items.compileSequence,
                                                                  items: items.allItems,
                                                                  onlyCompileTypeScriptForModules: compilerConfig.onlyCompileTypeScriptForModules)

        // parse the TS file annotations
        let typeScriptItems = userResult.typeScriptItems
        let filteredTypeScriptItems = typeScriptItems.filter { !$0.item.bundleInfo.disableAnnotationProcessing }
        let outItems = try dumpSymbols(typescriptItems: filteredTypeScriptItems, items: userResult.outItems)

        return CompilationItems(compileSequence: items.compileSequence, items: outItems)
    }

    private func dumpSymbols(typescriptItems: [TypeScriptItem], items: [CompilationItem]) throws -> [CompilationItem] {
        logger.info("Getting TypeScript comments...")
        let dumpAllSymbolsResult = try typeScriptCompilerManager.dumpAllSymbols(typescriptItems: typescriptItems)

        let dumpedSymbolsItems = dumpAllSymbolsResult.typeScriptItemsAndSymbols.map { typeScriptItemAndSymbols in
            return typeScriptItemAndSymbols.typeScriptItem.item.with(newKind: .dumpedTypeScriptSymbols(DumpedTypeScriptSymbolsResult(typeScriptItemAndSymbols: typeScriptItemAndSymbols)))
        }
        return items + dumpedSymbolsItems
    }
}
