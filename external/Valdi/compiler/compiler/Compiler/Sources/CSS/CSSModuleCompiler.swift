//
//  CSSModuleGenerator.swift
//  
//
//  Created by Simon Corsin on 10/29/19.
//

import Foundation

struct CSSModuleCompilerOutputFile {
    let file: File
    let outputPath: String
}

struct CSSModuleCompilerResult {
    let compiledCSS: CSSModuleCompilerOutputFile
    let jsModule: CSSModuleCompilerOutputFile
    let typeDefinition: CSSModuleCompilerOutputFile
    let allOpenedFiles: Set<URL>
}

class CSSModuleCompiler {

    private let logger: ILogger
    private let fileContent: String
    private let projectBaseDir: URL
    private let relativeProjectPath: String

    init(logger: ILogger, fileContent: String, projectBaseDir: URL, relativeProjectPath: String) {
        self.logger = logger
        self.fileContent = fileContent
        self.projectBaseDir = projectBaseDir
        self.relativeProjectPath = relativeProjectPath
    }

    private func generateModule(importPath: String, ruleIndex: Valdi_CSSRuleIndex) -> (typeDefinition: String, jsContent: String) {
        let classRules = ruleIndex.classRules.sorted { (left, right) -> Bool in
            return left.name < right.name
        }

        let tsOutput = CodeWriter()
        tsOutput.appendBody("""
        /**
         * CSS Module generated from \(relativeProjectPath)
         */
        \n
        """)

        var jsDeclarationLines = [String]()

        for namedRule in classRules {
            let className = namedRule.name
            let tsName = className.camelCased

            tsOutput.appendBody("")
            tsOutput.appendBody("export const \(tsName): {\n")

            for style in namedRule.node.styles {
                let attributeValue: String
                if !style.attribute.strValue.isEmpty {
                    attributeValue = "'\(style.attribute.strValue)'"
                } else if style.attribute.intValue != 0 {
                    attributeValue = "\(style.attribute.intValue)"
                } else {
                    attributeValue = "\(style.attribute.doubleValue)"
                }
                tsOutput.appendBody("  \(style.attribute.name): \(attributeValue);\n")
            }

            tsOutput.appendBody("}\n\n")

            jsDeclarationLines.append("'\(tsName)': '\(className)'")
        }

        // TODO(3521): Update to valdi
        let jsOutput = """
        module.exports = require('valdi_core/src/CSSModule').makeModule('\(importPath)', {\(jsDeclarationLines.joined(separator: ", "))});\n
        """

        return (typeDefinition: tsOutput.content.indented(indentPattern: "  "), jsContent: jsOutput)
    }

    func compile() throws -> CSSModuleCompilerResult {
        let baseURL = projectBaseDir.appendingPathComponent(relativeProjectPath).deletingLastPathComponent()
        let compiler = StyleSheetCompiler(logger: logger, content: fileContent, baseURL: baseURL, relativeProjectPath: relativeProjectPath)
        let result = try compiler.compile()

        let data = try result.rootStyleNode.orderedSerializedData()

        let importPath = "\(relativeProjectPath).\(FileExtensions.valdiCss)"

        let compiledCSS = CSSModuleCompilerOutputFile(file: .data(data), outputPath: importPath)

        let module = generateModule(importPath: importPath, ruleIndex: result.rootStyleNode.ruleIndex)

        let jsModule = CSSModuleCompilerOutputFile(file: .data(try module.jsContent.utf8Data()), outputPath: "\(relativeProjectPath).js")

        let typeDefinitionPath = "\(relativeProjectPath).\(FileExtensions.typescriptDeclaration)"
        let typeDefinition = CSSModuleCompilerOutputFile(file: .data(try module.typeDefinition.utf8Data()), outputPath: typeDefinitionPath)

        return CSSModuleCompilerResult(compiledCSS: compiledCSS, jsModule: jsModule, typeDefinition: typeDefinition, allOpenedFiles: compiler.filesImported)
    }
}
