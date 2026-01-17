//
//  KotlinViewClassGenerator.swift
//  Compiler
//
//  Created by Simon Corsin on 5/17/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

import Foundation

class KotlinViewClassGenerator: LanguageSpecificViewClassGenerator {
    static let platform: Platform = .android

    private let bundleInfo: CompilationItem.BundleInfo

    private let className: JVMClass
    private let componentPath: String

    private let sourceFilename: GeneratedSourceFilename

    private let viewModelClassName: String?
    private let componentContextClassName: String?

    private let writer: KotlinCodeGenerator

    private let staticMethods = KotlinCodeGenerator()

    init(bundleInfo: CompilationItem.BundleInfo,
         className: String,
         componentPath: String,
         sourceFilename: GeneratedSourceFilename,
         viewModelClassName: String?,
         componentContextClassName: String?) {
        self.bundleInfo = bundleInfo
        self.className = JVMClass(fullClassName: className)
        self.componentPath = componentPath
        self.sourceFilename = sourceFilename
        self.viewModelClassName = viewModelClassName
        self.componentContextClassName = componentContextClassName
        self.writer = KotlinCodeGenerator(package: self.className.package)
    }

    func start() throws {
        writer.appendHeader("")

        try [
            "com.snap.valdi.IValdiRuntime",
            "com.snap.valdi.context.ValdiViewOwner",
            "com.snap.valdi.views.ValdiGeneratedRootView"
            ].forEach { try self.writer.importClass($0) }

        writer.appendBody(FileHeaderCommentGenerator.generateComment(sourceFilename: sourceFilename))

        let viewModelType = try getViewModelType()
        let componentContextType = try getComponentContextType()

        writer.appendBody("\nclass \(className.name)(context: android.content.Context): ValdiGeneratedRootView<\(viewModelType), \(componentContextType)>(context) {\n\n")

        staticMethods.appendBody("companion object {\n")
    }

    func finish() {
        staticMethods.appendBody("}\n")

        writer.appendBody(staticMethods)

        writer.appendBody("}\n")
    }

    func write() throws -> [NativeSource] {
        let data = try writer.content.indented(indentPattern: "    ").utf8Data()
        return [KotlinCodeGenerator.makeNativeSource(bundleInfo: bundleInfo, jvmClass: className, file: .data(data))]
    }

    func appendAccessor(className: String, nodeId: String) throws {
        let jvmClass = try writer.importClass(className)

        let staticPropertyName = "\(nodeId.camelCased)Id"

        staticMethods.appendBody("val \(staticPropertyName) = \"\(nodeId)\"\n")

        writer.appendBody("val \(nodeId.camelCased): \(jvmClass.name)?\n")
        writer.appendBody("get() = valdiContext?.getView(\(staticPropertyName)) as? \(jvmClass.name)\n")
        writer.appendBody("\n\n")
    }

    private func getViewModelType() throws -> String {
        guard let viewModelClassName = viewModelClassName else {
            return "Any?"
        }
        return try writer.importClass(viewModelClassName).name
    }

    private func getComponentContextType() throws -> String {
        guard let componentContextClassName = componentContextClassName else {
            return "Any"
        }
        return try writer.importClass(componentContextClassName).name
    }

    func appendConstructor() throws {
        let viewModelType = try getViewModelType()
        let componentContextType = try getComponentContextType()

        let inflationCompletion = try writer.importClass("com.snap.valdi.InflationCompletion")

        staticMethods.appendBody("val componentPath = \"\(componentPath)\"\n")
        staticMethods.appendBody("\n")

        staticMethods.appendBody("@JvmStatic fun create(runtime: IValdiRuntime, owner: ValdiViewOwner? = null): \(className.name) = create(runtime, null, null, owner)\n\n")
        staticMethods.appendBody("""
        @JvmStatic fun create(runtime: IValdiRuntime, viewModel: \(viewModelType)?, componentContext: \(componentContextType)?, owner: ValdiViewOwner? = null, inflationCompletion: \(inflationCompletion.name)? = null): \(className.name) {
            val view = \(className.name)(runtime.context)
            runtime.inflateViewAsync(view, componentPath, viewModel, componentContext, owner, inflationCompletion)
            return view
        }
        \n
        """)
    }

    func appendNativeActions(nativeFunctionNames: [String]) {
        guard !nativeFunctionNames.isEmpty else {
            return
        }

        let keepName = try! writer.importClass("androidx.annotation.Keep").name
        writer.appendBody("@\(keepName)\n")
        writer.appendBody("interface ActionHandler {\n")
        for functionName in nativeFunctionNames {
            writer.appendBody("fun \(functionName)(parameters: Array<Any?>)\n")
        }
        writer.appendBody("}\n\n")

        writer.appendBody("""
        var actionHandler: ActionHandler?
            get() = valdiContext?.actionHandler as? ActionHandler
            set(value) { setActionHandlerUntyped(value) }
        \n
        """)
    }

    func appendEmitActions(actions: [String]) {
        for action in actions {
            let functionName = String(action.split(separator: "_").last!).pascalCased

            writer.appendBody("""
                fun emit\(functionName)(parameters: Array<Any?> = arrayOf()) {
                    getValdiContext {
                        it.performJsAction(\"\(action)\", parameters)
                    }
                }
                \n
            """)
        }
    }

}
