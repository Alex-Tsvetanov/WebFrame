//
//  CompileTypeScriptProcessor.swift
//  Compiler
//
//  Created by saniul on 13/06/2019.
//

import Foundation

final class CompileTypeScriptProcessor: CompilationProcessor {

    var description: String {
        return "Compiling TypeScript"
    }

    private let typeScriptCompilerManager: TypeScriptCompilerManager
    private let compilerConfig: CompilerConfig

    init(typeScriptCompilerManager: TypeScriptCompilerManager, compilerConfig: CompilerConfig) {
        self.typeScriptCompilerManager = typeScriptCompilerManager
        self.compilerConfig = compilerConfig
    }

    struct ComponentPathData {
        let componentClass: String
        let componentPath: ComponentPath
    }

    private func injectComponentPaths(selectedItem: SelectedItem<(JavaScriptFile, ComponentPathData)>) throws -> CompilationItem {
        var jsContent = try selectedItem.data.0.file.readString()

        let stringToInsert =  "\n\(selectedItem.data.1.componentClass).componentPath = \"\(selectedItem.data.1.componentPath.stringRepresentation)\";\n"

        let insertionIndex = jsContent.range(of: SourceMapProcessor.sourceMapPrefix)?.lowerBound ?? jsContent.endIndex
        jsContent.insert(contentsOf: stringToInsert, at: insertionIndex)

        let modifiedJsContent = try jsContent.utf8Data()

        return selectedItem.item.with(newKind: .javaScript(selectedItem.data.0.with(file: .data(modifiedJsContent))))
    }

    func process(items: CompilationItems) throws -> CompilationItems {
        // We add a Valdi.ignore.ts file to the root of valdi_modules, so that we can get the TypeScript compiler
        // to open and parse any .d.ts files we specify. Unfortunately, the TypeScript compiler does not pick up
        // projects that only specify .d.ts files.
        //
        // ignore files ending in .ignore.ts
        let selectedItems = items.allItems.filter { !$0.sourceURL.lastPathComponent.hasSuffix(".ignore.ts") }

        // compile the TS files
        let generatedResult = try typeScriptCompilerManager.compileItems(compileSequence: items.compileSequence,
                                                                         items: selectedItems,
                                                                         onlyCompileTypeScriptForModules: compilerConfig.onlyCompileTypeScriptForModules)

        var out = CompilationItems(compileSequence: items.compileSequence, items: generatedResult.outItems)

        // inject the componentPath into all components, so that we can retrieve them within JS

        // Associate component name by which files they live in
        let componentNameByScriptURL =  out.allItems.compactMap { (item) -> (URL, ComponentPathData)? in
            if case .document(let result) = item.kind {
                if let userScriptSourceURL = result.userScriptSourceURL, let componentClass = result.originalDocument.template?.jsComponentClass {
                    return (userScriptSourceURL, ComponentPathData(componentClass: componentClass, componentPath: result.componentPath))
                }
            }
            return nil
        }.associate { keyVal -> (URL, ComponentPathData) in
            return keyVal
        }

        out = try out.select { (item) -> (JavaScriptFile, ComponentPathData)? in
            guard let componentName = componentNameByScriptURL[item.sourceURL] else { return nil }

            if case .javaScript(let file) = item.kind {
                return (file, componentName)
            }

            return nil
        }.transformEachConcurrently(injectComponentPaths)

        return out
    }

}
