//
//  KotlinViewClassGenerator.swift
//  Compiler
//
//  Copyright © 2024 Snap Inc. All rights reserved.
//

import Foundation

class SwiftViewClassGenerator: LanguageSpecificViewClassGenerator {
    static let platform: Platform = .ios

    private let bundleInfo: CompilationItem.BundleInfo
    private let iosType: IOSType
    private let componentPath: String

    private let sourceFilename: GeneratedSourceFilename

    private let viewModelClass: IOSType?
    private let componentContextClass: IOSType?

    private let writer: SwiftSourceFileGenerator

    init(bundleInfo: CompilationItem.BundleInfo,
         iosType: IOSType,
         componentPath: String,
         sourceFilename: GeneratedSourceFilename,
         viewModelClass: IOSType?,
         componentContextClass: IOSType?) {
        self.bundleInfo = bundleInfo
        self.iosType = iosType
        self.componentPath = componentPath
        self.sourceFilename = sourceFilename
        self.viewModelClass = viewModelClass
        self.componentContextClass = componentContextClass
        self.writer = SwiftSourceFileGenerator(className: iosType.swiftName)
    }

    func start() throws {
        // TODO(3521): update to valdi_core
        self.writer.addImport(path: "valdi_core")
        self.writer.addImport(path: "ValdiCoreSwift")
        writer.appendBody(FileHeaderCommentGenerator.generateComment(sourceFilename: sourceFilename))
        writer.appendBody("\npublic class \(iosType.swiftName): SwiftComponentBase {\n\n")
    }

    func finish() {
        writer.appendBody("}\n")
    }

    func write() throws -> [NativeSource] {
        let data = try writer.content.indented(indentPattern: "    ").utf8Data()
        return [NativeSource(relativePath: nil,
                             filename: "\(iosType.swiftName).\(FileExtensions.swift)",
                             file: .data(data),
                             groupingIdentifier: "\(bundleInfo.iosModuleName).\(FileExtensions.swift)",
                             groupingPriority: IOSType.viewTypeGroupingPriority
                            )]
    }

    func appendAccessor(className: String, nodeId: String) throws {
        let staticPropertyName = "\(nodeId.camelCased)Id"

        writer.appendBody("let \(nodeId.camelCased): \(className)? {\n")
        writer.appendBody("get { return valdiContext?.getView(\(staticPropertyName)) as? \(className) }\n")
        writer.appendBody("\n\n")
    }

    private func getViewModelType() -> String? {
        guard let viewModelClassName = viewModelClass?.swiftName else {
            return nil
        }
        return viewModelClassName
    }

    private func getComponentContextType() -> String? {
        guard let componentContextClassName = componentContextClass?.swiftName else {
            return nil
        }
        return componentContextClassName
    }
    
    func appendNativeActions(nativeFunctionNames: [String]) {
        // Not supported
    }
    
    func appendEmitActions(actions: [String]) {
        // Not supported
    }

    func appendConstructor() throws {
        writer.appendBody(
        """
        static public override func componentPath() -> String {
            return "\(componentPath)"
        }
        \n
        """)

        
    }
}
