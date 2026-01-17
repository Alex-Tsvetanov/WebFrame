//
//  CompileDocumentsProcessor.swift
//  Compiler
//
//  Created by Simon Corsin on 7/24/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

import Foundation
import SWXMLHash
import SwiftSoup

private extension CompilationItem {
    var documentNameFromSourceURL: String {
        return sourceURL.deletingPathExtension().lastPathComponent
    }
}

// [.parsedDocument, .css] -> [.document, .invalidDocument, .typeScript]
final class CompileDocumentsProcessor: CompilationProcessor {

    var description: String {
        return "Compiling Documents"
    }

    private let logger: ILogger
    private var projectClassMappingManager: ProjectClassMappingManager
    private let userScriptManager: UserScriptManager
    private let fileDependenciesManager: FileDependenciesManager

    init(logger: ILogger, projectClassMappingManager: ProjectClassMappingManager, userScriptManager: UserScriptManager, fileDependenciesManager: FileDependenciesManager) {
        self.logger = logger
        self.projectClassMappingManager = projectClassMappingManager
        self.userScriptManager = userScriptManager
        self.fileDependenciesManager = fileDependenciesManager
    }

    private func compile(item: CompilationItem, template: ValdiRawTemplate, actions: [ValdiAction], viewModel: ValdiModel?, classMapping: ResolvedClassMapping, documentContent: String) throws -> TemplateCompilerResult {
        let templateCompiler = ValdiTemplateCompiler(template: template, viewModel: viewModel, classMapping: classMapping, actions: actions, compileIOS: true, compileAndroid: true, documentContent: documentContent)
        return try templateCompiler.compile()
    }

    private func getComponentPath(item: CompilationItem) -> ComponentPath {
        // TODO(simon): Remove this hack
        return ComponentPath(fileName: "\(item.relativeProjectPath).generated", exportedMember: "ComponentClass")
    }

    private func resolveClassMapping(item: CompilationItem, rawDocument: ValdiRawDocument) throws {
        if let rootNode = rawDocument.template?.rootNode {
            let rootNodeType = rootNode.nodeType
            let isLayout = rootNode.isLayout ?? true

            let rootClassMapping = rawDocument.classMapping?.nodeMappingByClass[rootNodeType]

            let viewPath = getComponentPath(item: item)

            do {
                try projectClassMappingManager.mutate { (classMapping) in
                    // TODO(simon): Remove this hack
                    let rootClass: ValdiClass
                    if isLayout {
                        rootClass = try classMapping.resolveClass(nodeType: "Layout", currentBundle: item.bundleInfo, localMapping: nil)
                    } else {
                        rootClass = try classMapping.resolveClass(nodeType: "View", currentBundle: item.bundleInfo, localMapping: nil)
                    }

                    try classMapping.register(bundle: item.bundleInfo,
                                              nodeType: rootNodeType,
                                              valdiViewPath: viewPath,
                                              iosType: rootClassMapping?.iosType,
                                              androidClassName: rootClassMapping?.androidClassName,
                                              jsxName: rootClass.jsxName,
                                              isLayout: isLayout,
                                              sourceFilePath: item.relativeBundleURL.path)
                }
            } catch let error {
                throw CompilerError(type: "ClassMapping Error", message: error.legibleLocalizedDescription, atZeroIndexedLine: rootNode.lineNumber, column: rootNode.columnNumber, inDocument: rawDocument.content)
            }
        }
    }

    private func resolveClassMapping(selectedItem: SelectedItem<File>) throws {
        try resolveClassMapping(item: selectedItem.item, classMappingXMLFile: selectedItem.data)
    }

    private func resolveClassMapping(item: CompilationItem, classMappingXMLFile: File) throws {
        let parser = SwiftSoup.Parser.xmlParser()
        parser.settings(.preserveCase)
        parser.setTrackErrors(16)

        let xml = try parser.parseInput(try classMappingXMLFile.readString(), "")

        guard let rootElement = xml.children().first(), rootElement.tagName() == "class-mapping" else {
            throw CompilerError("Root element of a class mapping file should be 'class-mapping'")
        }

        let localClassMapping = try ValdiDocumentParser.parseClassMapping(classMappingElement: rootElement, iosImportPrefix: item.bundleInfo.iosModuleName)
        try projectClassMappingManager.mutate { (classMapping) in
            for (nodeType, mapping) in localClassMapping.nodeMappingByClass {
                try classMapping.register(bundle: item.bundleInfo, nodeType: nodeType, valdiViewPath: nil, iosType: mapping.iosType, androidClassName: mapping.androidClassName, jsxName: nil, isLayout: false, isSlot: false, sourceFilePath: item.relativeBundleURL.path)
            }
        }
    }

    private func compile(item: CompilationItem, rawDocument: ValdiRawDocument) -> [CompilationItem] {
        let inputURL = item.sourceURL
        logger.debug("-- Compiling Document \(inputURL.lastPathComponent)")
        do {
            guard let template = rawDocument.template else {
                throw CompilerError("No template found in this document")
            }

            let projectClassMapping = projectClassMappingManager.copyProjectClassMapping()
            let resolvedMapping = ResolvedClassMapping(localClassMapping: rawDocument.classMapping,
                                                       projectClassMapping: projectClassMapping,
                                                       currentBundle: item.bundleInfo)

            let compiledTemplate = try compile(item: item,
                                               template: template,
                                               actions: rawDocument.actions,
                                               viewModel: rawDocument.viewModel,
                                               classMapping: resolvedMapping,
                                               documentContent: rawDocument.content)

            var out = [CompilationItem]()

            let scriptLang = FileExtensions.typescript

            let userScriptURL = userScriptManager.findUserScriptURL(documentSourceURL: inputURL)

            let componentPath = getComponentPath(item: item)

            let compilationResult = CompilationResult(
                componentPath: componentPath,
                originalDocument: rawDocument,
                templateResult: compiledTemplate,
                classMapping: resolvedMapping,
                userScriptSourceURL: userScriptURL,
                scriptLang: scriptLang,
                symbolsToImportsInGeneratedCode: [])

            out.append(item.with(newKind: .document(compilationResult)))

            return out
        } catch let error {
            logger.error("Failed to compile document \(inputURL): \(error.legibleLocalizedDescription)")
            return [item.with(error: error)]
        }
    }

    private func compile(selectedItem: SelectedItem<ValdiRawDocument>) throws -> [CompilationItem] {
        return compile(item: selectedItem.item, rawDocument: selectedItem.data)
    }

    func process(items: CompilationItems) throws -> CompilationItems {
        // First resolve the class mappings

       try items.select { (item) in
            switch item.kind {
            case .classMapping(let file):
                return file
            default:
                return nil
            }
        }.processEachConcurrently(resolveClassMapping)

        var filteredItems = [CompilationItem]()

        for item in items.allItems {
            switch item.kind {
            case .classMapping:
                break
            case .css:
                // css files get injected into the parsed documents directly.
                break
            case .parsedDocument(let parsedDocument):
                do {
                    try resolveClassMapping(item: item, rawDocument: parsedDocument)
                    filteredItems.append(item)
                } catch let error {
                    filteredItems.append(item.with(error: error))
                }
            default:
                filteredItems.append(item)
            }
        }

        return try CompilationItems(compileSequence: items.compileSequence, items: filteredItems).select {
            if case .parsedDocument(let parsedDocument) = $0.kind {
                return parsedDocument
            } else {
                return nil
            }
        }.transformEachConcurrently(compile)
    }

}
