//
//  ObjCViewModelGenerator.swift
//  Compiler
//
//  Created by Simon Corsin on 5/17/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

import Foundation

final class ObjCEnumGenerator {
    private let iosType: IOSType
    private let typeName: String
    private let exportedEnum: ExportedEnum
    private let classMapping: ResolvedClassMapping
    private let sourceFileName: GeneratedSourceFilename
    private let bundleInfo: CompilationItem.BundleInfo

    init(iosType: IOSType, exportedEnum: ExportedEnum, classMapping: ResolvedClassMapping, sourceFileName: GeneratedSourceFilename, bundleInfo: CompilationItem.BundleInfo) {
        self.iosType = iosType
        self.typeName = iosType.name
        self.exportedEnum = exportedEnum
        self.classMapping = classMapping
        self.sourceFileName = sourceFileName
        self.bundleInfo = bundleInfo
    }

    private func containsOnlyIncreasingValues(enumCases: [EnumCase<Int>]) -> Bool {
        let set = Set(enumCases.map { $0.value })

        if set.count != enumCases.count {
            return false
        }

        for index in 0..<enumCases.count {
            if !set.contains(index) {
                return false
            }
        }

        return true
    }

    static func nullableTypeName(typename: String) -> String {
        return "\(typename)_Nullable"
    }

    private static func generateEnumCaseString(declarationString: String, comments: String?) -> String {
        if let comments = comments {
            let multilineComments = FileHeaderCommentGenerator.generateMultilineComment(comment: comments)
            return "\(multilineComments)\n\(declarationString)"
        } else {
            return declarationString
        }
    }

    private func generateCode() throws -> GeneratedCode {
        let classGenerator = ObjCClassGenerator(className: typeName)

        classGenerator.header.addImport(path: "<Foundation/Foundation.h>")
        // TODO(3521): Update to valdi_core
        classGenerator.header.addImport(path: "<valdi_core/SCValdiMarshaller.h>")
        classGenerator.apiImpl.addImport(path: "<valdi_core/SCValdiMarshallableObjectRegistry.h>")
        classGenerator.impl.addImport(path: "<valdi_core/SCValdiMarshallableObjectRegistry.h>")
        classGenerator.impl.addImport(path: "<valdi_core/SCValdiMarshallableObject.h>")

        classGenerator.impl.addImport(path: iosType.importHeaderStatement(kind: .withUtilities))
        classGenerator.impl.appendBody(FileHeaderCommentGenerator.generateComment(sourceFilename: sourceFileName))
        classGenerator.impl.appendBody("\n\n")

        classGenerator.header.appendBody(FileHeaderCommentGenerator.generateComment(sourceFilename: sourceFileName))
        classGenerator.header.appendBody("\n")

        let comments = FileHeaderCommentGenerator.generateComment(sourceFilename: sourceFileName, additionalComments: exportedEnum.comments)
        classGenerator.apiHeader.appendBody(comments)
        classGenerator.apiHeader.appendBody("\n")

        switch exportedEnum.cases {
        case .enum(let cases):
            let objcEnumCases = cases.map {
                let caseName = typeName + $0.name
                let declaration = "\(caseName) = \($0.value)"
                return ObjCEnumGenerator.generateEnumCaseString(declarationString: declaration, comments: $0.comments)
            }.joined(separator: ",\n")

            classGenerator.apiHeader.appendBody("""
                typedef NS_CLOSED_ENUM(int32_t, \(typeName)) {
                    \(objcEnumCases)
                }
                """)

            classGenerator.apiHeader.appendBody(";\n")
            classGenerator.apiHeader.appendBody("""
                NSArray<NSNumber *> * _Nonnull \(typeName)GetAllCases();\n
            """)


            if containsOnlyIncreasingValues(enumCases: cases) {
                classGenerator.apiImpl.appendBody("""

                VALDI_TRIVIAL_INT_ENUM(\(typeName), \(cases.count))

                """)
            } else {
                let allEnumCasesBody = cases.map { typeName + $0.name }.joined(separator: ", ")
                classGenerator.apiImpl.appendBody("""

                VALDI_INT_ENUM(\(typeName), \(allEnumCasesBody))

                """)
            }

            classGenerator.apiImpl.appendBody("\n")
            classGenerator.apiImpl.appendBody("""
            NSArray<NSNumber *> * _Nonnull \(typeName)GetAllCases() {
                static dispatch_once_t onceToken;
                static NSArray<NSNumber *> *kAllCases;
                dispatch_once(&onceToken, ^{
                    kAllCases = @[\(cases.map { "@\($0.value)" }.joined(separator: ", "))];
                });
                return kAllCases;
            }
            """)
        case .stringEnum(let cases):
            let objcEnumCases = cases.map {
                let caseName = typeName + $0.name
                let declaration = "FOUNDATION_EXPORT \(typeName) \(caseName);\n"
                return ObjCEnumGenerator.generateEnumCaseString(declarationString: declaration, comments: $0.comments)
            }.joined()

            classGenerator.apiHeader.appendBody("typedef NSString * _Nonnull \(typeName) NS_STRING_ENUM")
            classGenerator.apiHeader.appendBody(";\n")

            classGenerator.apiHeader.appendBody("typedef NSString * _Nullable \(ObjCEnumGenerator.nullableTypeName(typename: typeName)) NS_STRING_ENUM")
            classGenerator.apiHeader.appendBody(";\n")

            classGenerator.apiHeader.appendBody("""

                \(objcEnumCases)
            """)
            classGenerator.apiHeader.appendBody("\n")
            classGenerator.apiHeader.appendBody("""
                NSArray<NSString *> * _Nonnull \(typeName)GetAllCases();
            """)

            let objcEnumCaseNames = cases.map {
                typeName + $0.name
            }

            let objcEnumCasesImpl = cases.map {
                let caseName = typeName + $0.name
                return "\(typeName) \(caseName) = @\"\($0.value)\";\n"
                }.joined()

            classGenerator.apiImpl.appendBody("""
            \(objcEnumCasesImpl)
            """)

            let allEnumCasesBody = objcEnumCaseNames.joined(separator: ", ")
            classGenerator.apiImpl.appendBody("""
            VALDI_STRING_ENUM(\(typeName), \(allEnumCasesBody))
            """)

            classGenerator.apiImpl.appendBody("\n")
            classGenerator.apiImpl.appendBody("""
            NSArray<NSString *> * _Nonnull \(typeName)GetAllCases() {
                static dispatch_once_t onceToken;
                static NSArray<NSString *> *kAllCases;
                dispatch_once(&onceToken, ^{
                    kAllCases = @[\(allEnumCasesBody)];
                });
                return kAllCases;
            }
            """)
        }

        return GeneratedCode(apiHeader: classGenerator.apiHeader, apiImpl: classGenerator.apiImpl, header: classGenerator.header, impl: classGenerator.impl)
    }

    func write() throws -> [NativeSource] {
        let generatedCode = try generateCode()
        let nativeSources = try NativeSource.iosNativeSourcesFromGeneratedCode(generatedCode,
                                                                               iosType: iosType,
                                                                               bundleInfo: bundleInfo)
        return nativeSources
    }

}
