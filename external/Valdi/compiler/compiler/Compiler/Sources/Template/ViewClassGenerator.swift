//
//  ViewClassGenerator.swift
//  Compiler
//
//  Created by Simon Corsin on 4/12/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

import Foundation

struct ViewClassGenerationResult {
    let nativeSources: [NativeSourceAndPlatform]
    let description: GeneratedViewClassDescription
}

final class ViewClassGenerator {

    private let bundleInfo: CompilationItem.BundleInfo
    private let logger: ILogger
    private let compiledDocument: TemplateCompilerResult
    private let rawDocument: ValdiRawDocument
    private let componentPath: ComponentPath?
    private let sourceFilename: GeneratedSourceFilename
    private let iosType: IOSType?
    private let iosLanguage: IOSLanguage
    private let androidClassName: String?

    init(bundleInfo: CompilationItem.BundleInfo,
         logger: ILogger,
         compiledDocument: TemplateCompilerResult,
         rawDocument: ValdiRawDocument,
         componentPath: ComponentPath?,
         sourceFilename: GeneratedSourceFilename,
         iosType: IOSType?,
         iosLanguage: IOSLanguage,
         androidClassName: String?) {
        self.bundleInfo = bundleInfo
        self.logger = logger
        self.compiledDocument = compiledDocument
        self.rawDocument = rawDocument
        self.componentPath = componentPath
        self.sourceFilename = sourceFilename
        self.iosType = iosType
        self.iosLanguage = iosLanguage
        self.androidClassName = androidClassName
    }

    private func generateAccessors(nodes: [(raw: ValdiRawNode, compiled: ValdiJSElement)], generator: LanguageSpecificViewClassGenerator) {
        for (rawNode, compiledNode) in nodes {
            let className: String?
            switch type(of: generator).platform {
            case .ios: className = compiledNode.iosViewClass
            case .android: className = compiledNode.androidViewClass
            // This won't happen yet
            case .web: className = nil
            case .cpp: className = nil
            }

            if let className = className {
                do {
                    try generator.appendAccessor(className: className, nodeId: rawNode.id)
                } catch let error {
                    let platform = type(of: generator).platform
                    logger.error("Failed to add \(platform) accessor for node \(rawNode.id): \(error.legibleLocalizedDescription)")
                }
            }
        }
    }

    private func listAccessorEligibleNodes(rawNode: ValdiRawNode?, compiledNode: ValdiJSElement) -> [(raw: ValdiRawNode, compiled: ValdiJSElement)] {
        guard let rawNode = rawNode else {
            return []
        }

        var nodes: [(raw: ValdiRawNode, compiled: ValdiJSElement)] = []

        if !rawNode.id.isEmpty && compiledNode.jsxName != "layout" {
            nodes.append((rawNode, compiledNode))
        }

        let childNodes = rawNode.children.enumerated().flatMap { (index, child) -> [(raw: ValdiRawNode, compiled: ValdiJSElement)] in
            guard let childNode = child.node, let childCompiledNode = compiledNode.children[index].element else {
                return []
            }
            return listAccessorEligibleNodes(rawNode: childNode, compiledNode: childCompiledNode)
        }

        nodes += childNodes

        return nodes
    }

    func generate() throws -> ViewClassGenerationResult? {
        var generators: [LanguageSpecificViewClassGenerator] = []

        guard let componentPath = self.componentPath else {
            throw CompilerError("Could not resolve componentPath")
        }

        if let iosType = iosType {
            let iosSwiftGenerator = SwiftViewClassGenerator(bundleInfo: self.bundleInfo,
                                                            iosType: iosType,
                                                            componentPath: componentPath.stringRepresentation,
                                                            sourceFilename: sourceFilename,
                                                            viewModelClass: rawDocument.viewModel?.iosType,
                                                            componentContextClass: rawDocument.componentContext?.iosType)
            let iosObjCGenerator = ObjCViewClassGenerator(bundleInfo: self.bundleInfo,
                                                          iosType: iosType,
                                                          componentPath: componentPath.stringRepresentation,
                                                          sourceFilename: sourceFilename,
                                                          viewModelClass: rawDocument.viewModel?.iosType,
                                                          componentContextClass: rawDocument.componentContext?.iosType)
            switch self.iosLanguage {
            case IOSLanguage.swift:
                generators.append(iosSwiftGenerator)
            case IOSLanguage.objc:
                generators.append(iosObjCGenerator)
            case IOSLanguage.both:
                generators.append(iosSwiftGenerator)
                generators.append(iosObjCGenerator)
            }
            logger.debug("Generating iOS type \(iosType.name)")
        } else {
            logger.debug("Not generating iOS type")
        }

        if let androidClassName = androidClassName {
            let androidCodeGenerator = KotlinViewClassGenerator(bundleInfo: self.bundleInfo,
                                                                className: androidClassName,
                                                                componentPath: componentPath.stringRepresentation,
                                                                sourceFilename: sourceFilename,
                                                                viewModelClassName: rawDocument.viewModel?.androidClassName,
                                                                componentContextClassName: rawDocument.componentContext?.androidClassName)
            generators.append(androidCodeGenerator)
            logger.debug("Generating Android class \(androidClassName)")
        } else {
            logger.debug("Not generating Android class")
        }

        guard !generators.isEmpty else {
            return nil
        }

        let accessorEligibleNodes = listAccessorEligibleNodes(rawNode: rawDocument.template?.rootNode,
                                                              compiledNode: compiledDocument.rootElement)

        let viewModel = rawDocument.viewModel
        let componentContext = rawDocument.componentContext
        let accessors = accessorEligibleNodes.map { (rawNode, compiledNode) in
            GeneratedViewClassDescription.AccessorDescription(iosTypeName: compiledNode.iosViewClass,
                                                              androidClassName: compiledNode.androidViewClass,
                                                              name: rawNode.id.camelCased)
        }
        let nativeActions = compiledDocument.actions.filter { $0.type == .native }.map { $0.name }
        let emitActions = compiledDocument.actions.filter { $0.type == .javaScript }.map { $0.name }

        var nativeSources: [NativeSourceAndPlatform] = []

        for generator in generators {
            try generator.start()

            try generator.appendConstructor()

            generateAccessors(nodes: accessorEligibleNodes,
                              generator: generator)

            generator.appendNativeActions(nativeFunctionNames: nativeActions)
            generator.appendEmitActions(actions: emitActions)

            generator.finish()

            nativeSources += try generator.write().map {
                NativeSourceAndPlatform(source: $0, platform: type(of: generator).platform)
            }
        }

        let description = GeneratedViewClassDescription(iosTypeName: iosType?.name,
                                                        androidClassName: androidClassName,
                                                        iosViewModelClassName: viewModel?.iosType?.name,
                                                        androidViewModelClassName: viewModel?.androidClassName,
                                                        iosComponentContextClassName: componentContext?.iosType?.name,
                                                        androidComponentContextClassName: componentContext?.androidClassName,
                                                        accessors: accessors.nonEmpty,
                                                        nativeActions: nativeActions.nonEmpty,
                                                        emitActions: emitActions.nonEmpty)

        let result = ViewClassGenerationResult(nativeSources: nativeSources,
                                               description: description)
        return result
    }

}
