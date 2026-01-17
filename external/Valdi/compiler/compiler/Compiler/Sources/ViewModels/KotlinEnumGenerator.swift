//
//  KotlinViewModelGenerator.swift
//  Compiler
//
//  Created by Simon Corsin on 5/17/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

import Foundation

final class KotlinEnumGenerator {
    private let bundleInfo: CompilationItem.BundleInfo
    private let exportedEnum: ExportedEnum
    private let classMapping: ResolvedClassMapping

    private let writer: KotlinCodeGenerator
    private let jvmClass: JVMClass
    private let sourceFileName: GeneratedSourceFilename

    init(bundleInfo: CompilationItem.BundleInfo,
         fullTypeName: String,
         exportedEnum: ExportedEnum,
         classMapping: ResolvedClassMapping, 
         sourceFileName: GeneratedSourceFilename) {
        self.bundleInfo = bundleInfo
        self.exportedEnum = exportedEnum
        self.classMapping = classMapping

        jvmClass = JVMClass(fullClassName: fullTypeName)
        self.writer = KotlinCodeGenerator(package: jvmClass.package, classMapping: classMapping)
        self.sourceFileName = sourceFileName
    }

    func write() throws -> [NativeSource] {
        try write(className: jvmClass.name, writer: writer)

        let data = try writer.content.indented(indentPattern: "    ").utf8Data()

        return [KotlinCodeGenerator.makeNativeSource(bundleInfo: bundleInfo, jvmClass: jvmClass, file: .data(data))]
    }

    private func write<T>(className: String,
                          writer: KotlinCodeGenerator,
                          enumType: String,
                          objectType: KotlinMarshallableObjectDescriptorAnnotation.ObjectType,
                          cases: [EnumCase<T>],
                          getCaseStringValue: (Int, EnumCase<T>) -> String) throws {
        writer.appendBody(FileHeaderCommentGenerator.generateComment(sourceFilename: sourceFileName, additionalComments: exportedEnum.comments))
        writer.appendBody("\n")

        let importedEnumType = try writer.importClass("com.snap.valdi.schema.ValdiEnumType")
        let valdiEnumUtils = try writer.importClass("com.snap.valdi.utils.ValdiEnumUtils")

        let descriptorAnnotation = try writer.getDescriptorAnnotation(type: objectType, typeParameters: nil, additionalParameters: [
            ("type", "\(importedEnumType.name).\(enumType.uppercased())")
        ])

        writer.appendBody(descriptorAnnotation)
        writer.appendBody("enum class \(className) {\n")
        for (idx, c) in cases.enumerated() {
            let isLastCase = idx == cases.count - 1
            let commaOrSemicolon = isLastCase ? ";" : ","
            // NOTE: c.value already contains quotes
            if let comments = c.comments {
                writer.appendBody(FileHeaderCommentGenerator.generateMultilineComment(comment: comments))
                writer.appendBody("\n")
            }
            let fieldAnnotationContent = try descriptorAnnotation.appendField(propertyIndex: idx, propertyName: c.name, kotlinFieldName: c.name, expectedKotlinFieldName: c.name, schemaBuilder: { schemaWriter in
                let enumValue = getCaseStringValue(idx, c)
                schemaWriter.appendEnumValue(enumValue)
            })
            writer.appendBody(fieldAnnotationContent)
            writer.appendBody("\(c.name)\(commaOrSemicolon)\n")
        }

        writer.appendBody("\nval value: \(enumType)\n")
        writer.appendBody("get() = \(valdiEnumUtils.name).getEnum\(enumType)Value(this)\n\n")

        writer.appendBody("}\n")
    }

    private func write(className: String, writer: KotlinCodeGenerator) throws {
        switch exportedEnum.cases {
        case .enum(let cases):
            try write(className: className, writer: writer, enumType: "Int", objectType: KotlinMarshallableObjectDescriptorAnnotation.ObjectType.intEnum, cases: cases, getCaseStringValue: { _, c in
                return "\(c.value)"
            })
        case .stringEnum(let cases):
            try write(className: className, writer: writer, enumType: "String", objectType: KotlinMarshallableObjectDescriptorAnnotation.ObjectType.stringEnum, cases: cases, getCaseStringValue: { _, c in
                return "'\(c.value)'"
            })
        }
    }
}
