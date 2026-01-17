//
//  TypeScriptIntermediateUtils.swift
//  
//
//  Created by Simon Corsin on 3/23/20.
//

import Foundation

class TypeScriptIntermediateUtils {

    /**
     Emits a generated JS file along with its type definition.
     It will also ensure the type definition is available in the generated ts directory
     so that the IDE can pick it up.
     */
    static func emitGeneratedJs(fileManager: ValdiFileManager, projectConfig: ValdiProjectConfig, bundleInfo: CompilationItem.BundleInfo, outputTarget: OutputTarget, jsOutputPath: String, jsContent: File, tsDefinitionOutputPath: String, tsDefinitionContent: File) throws -> [CompilationItem] {
        let emittedJsSourceURL = bundleInfo.absoluteURL(forRelativeProjectPath: jsOutputPath)

        // Emit an item for our generated JS module
        let jsFile = JavaScriptFile(file: jsContent, relativePath: jsOutputPath)
        let emittedJsItem = CompilationItem(sourceURL: emittedJsSourceURL, relativeProjectPath: jsOutputPath, kind: .javaScript(jsFile), bundleInfo: bundleInfo, platform: nil, outputTarget: outputTarget)

        // Emit an item for the type definition

        let emittedTsSourceURL = bundleInfo.absoluteURL(forRelativeProjectPath: tsDefinitionOutputPath)
        let emittedTsItem = CompilationItem(sourceURL: emittedTsSourceURL, relativeProjectPath: tsDefinitionOutputPath, kind: .typeScript(tsDefinitionContent, emittedTsSourceURL), bundleInfo: bundleInfo, platform: nil, outputTarget: outputTarget)

        // Save the type declaration file to have autocompletion in VSCode
        let tsData = try tsDefinitionContent.readData()

        let definitionURL = projectConfig.generatedTsDirectoryURL.appendingPathComponent(tsDefinitionOutputPath)
        try fileManager.save(data: tsData, to: definitionURL)

        return [
            emittedJsItem,
            emittedTsItem
        ]
    }

    static func emitGeneratedJs(fileManager: ValdiFileManager, projectConfig: ValdiProjectConfig, sourceItem: CompilationItem, jsOutputPath: String, jsContent: File, tsDefinitionOutputPath: String, tsDefinitionContent: File) throws -> [CompilationItem] {
        return try emitGeneratedJs(fileManager: fileManager,
                                   projectConfig: projectConfig,
                                   bundleInfo: sourceItem.bundleInfo,
                                   outputTarget: sourceItem.outputTarget,
                                   jsOutputPath: jsOutputPath,
                                   jsContent: jsContent,
                                   tsDefinitionOutputPath: tsDefinitionOutputPath,
                                   tsDefinitionContent: tsDefinitionContent)
    }

}
