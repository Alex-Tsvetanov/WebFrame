//
//  SwiftEnumGenerator.swift
//  Compiler
//
//  Created by Ben Dodson on 2/8/2024
//  Copyright © 2024 Snap Inc. All rights reserved.
//

import Foundation

final class SwiftEnumGenerator {
    private let exportedEnum: ExportedEnum
    private let classMapping: ResolvedClassMapping

    private let writer: SwiftCodeGenerator
    private let sourceFileName: GeneratedSourceFilename

    private let iosType: IOSType
    private let typeName: String
    private let bundleInfo: CompilationItem.BundleInfo

    init(iosType: IOSType, exportedEnum: ExportedEnum, classMapping: ResolvedClassMapping, sourceFileName: GeneratedSourceFilename, bundleInfo: CompilationItem.BundleInfo) {
        self.iosType = iosType
        self.typeName = iosType.swiftName
        self.exportedEnum = exportedEnum
        self.classMapping = classMapping
        self.sourceFileName = sourceFileName
        self.bundleInfo = bundleInfo
        self.writer = SwiftCodeGenerator()
    }


    func write() throws -> [NativeSource] {
        try write(className: typeName, writer: writer)

        let data = try writer.content.indented(indentPattern: "    ").utf8Data()
        let sourcesDirectoryRelativePath = "../\(bundleInfo.iosModuleName)\(IOSType.HeaderImportKind.apiOnlyModuleNameSuffix)"
        return [NativeSource(relativePath: sourcesDirectoryRelativePath,
                             filename: "\(typeName).\(FileExtensions.swift)",
                             file: .data(data),
                             groupingIdentifier: "\(bundleInfo.iosModuleName)\(IOSType.HeaderImportKind.apiOnlyModuleNameSuffix).\(FileExtensions.swift)",
                             groupingPriority: IOSType.enumTypeGroupingPriority)]
    }

    private func write<T>(className: String,
                          writer: SwiftCodeGenerator,
                          enumType: String,
                          objectType: KotlinMarshallableObjectDescriptorAnnotation.ObjectType,
                          cases: [EnumCase<T>],
                          getCaseStringValue: (Int, EnumCase<T>) -> String) throws {
        guard enumType == "Int" || enumType == "String" else {
            throw CompilerError("Enum type \(enumType) is not supported in Swift. Only Int and String are supported.")
        }

        writer.appendBody(FileHeaderCommentGenerator.generateComment(sourceFilename: sourceFileName, additionalComments: exportedEnum.comments))
        writer.appendBody("\n")
        // TODO(3521): Update to ValdiCoreSwift
        writer.appendBody("import ValdiCoreSwift\n")

        let enumCasesBody = CodeWriter()
        let nameAllocator = PropertyNameAllocator.forSwift()
        for (idx, c) in cases.enumerated() {
            // NOTE: c.value already contains quotes
            if let comments = c.comments {
                enumCasesBody.appendBody(FileHeaderCommentGenerator.generateMultilineComment(comment: comments))
                enumCasesBody.appendBody("\n")
            }
            let actualName = nameAllocator.allocate(property: camelCase(c.name)).name
            let caseValue = getCaseStringValue(idx, c)
            enumCasesBody.appendBody("    case \(actualName) = \(caseValue)\n")
        }

        let nativeType = (enumType == "Int") ? "Int32" : enumType
        // TODO(3521): Update to ValdiMarshall*
        let valdiType = "ValdiMarshallable\(enumType)Enum"
        writer.appendBody(
        """
        public enum \(className): \(nativeType), \(valdiType) {
            \(enumCasesBody.content)
            public init(from marshaller: ValdiMarshaller, at objectIndex: Int) throws {
                self = try create\(enumType)Enum(from:marshaller, at:objectIndex)
            }
        }
        public func register_\(className)() {
            let enumCases: [\(nativeType)] = allEnumCases(of: \(className).self)
            ValdiMarshallableObjectRegistry.shared.register\(enumType)Enum(enumName: "\(className)", enumCases: enumCases)
        }\n
        """)
    }
    
    private func camelCase(_ string: String) -> String {
        let words = string.components(separatedBy: CharacterSet.alphanumerics.inverted)
        let camelCasedWords = words.enumerated().map { index, word -> String in
            if index == 0 {
                return word.lowercased()
            }
            return word.capitalized
        }
        
        return camelCasedWords.joined()
    }

    private func write(className: String, writer: SwiftCodeGenerator) throws {
        switch exportedEnum.cases {
        case .enum(let cases):
            try write(className: className, writer: writer, enumType: "Int", objectType: KotlinMarshallableObjectDescriptorAnnotation.ObjectType.intEnum, cases: cases, getCaseStringValue: { _, c in
                return "\(c.value)"
            })
        case .stringEnum(let cases):
            try write(className: className, writer: writer, enumType: "String", objectType: KotlinMarshallableObjectDescriptorAnnotation.ObjectType.stringEnum, cases: cases, getCaseStringValue: { _, c in
                return "\"\(c.value)\""
            })
        }
    }
}
