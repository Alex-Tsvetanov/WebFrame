//
//  TypeScriptAnnotatedSymbol.swift
//  BlueSocket
//
//  Created by Simon Corsin on 5/15/19.
//

import Foundation

class TypeScriptCommentedFile {

    let src: TypeScriptItemSrc
    let fileContent: String
    let linesIndexer: LinesIndexer

    let annotatedSymbols: [TypeScriptAnnotatedSymbol]
    private(set) var references: [TS.AST.TypeReference] = []

    init(src: TypeScriptItemSrc, file: File, dumpedSymbolsResult: TS.DumpSymbolsWithCommentsResponseBody, typeScriptCompiler: TypeScriptCompiler) throws {
        self.src = src
        self.fileContent = try file.readString()
        self.linesIndexer = LinesIndexer(str: fileContent)

        let dumpedSymbolsWithComments = dumpedSymbolsResult.dumpedSymbols
        self.references = dumpedSymbolsResult.references
        self.annotatedSymbols = try TypeScriptCommentedFile.resolveSymbolsAndAnnotations(dumpedSymbolsWithComments,
                                                                                         typeScriptCompiler: typeScriptCompiler,
                                                                                         fileContent: fileContent)
    }

    private static func resolveSymbolsAndAnnotations(_ dumpedSymbolWithCommentsResult: [TS.DumpedSymbolWithComments], typeScriptCompiler: TypeScriptCompiler, fileContent: String) throws -> [TypeScriptAnnotatedSymbol] {
        return try dumpedSymbolWithCommentsResult.map { dumpedSymbol in
            let annotatedSymbol = TypeScriptAnnotatedSymbol(symbol: dumpedSymbol)
            if let leadingComments = dumpedSymbol.leadingComments {
                let annotations = try ValdiTypeScriptAnnotation.extractAnnotations(comments: leadingComments,
                                                                                      fileContent: fileContent)
                annotatedSymbol.addAnnotations(annotations)

                if let interface = annotatedSymbol.symbol.interface {
                    for (idx, member) in interface.members.enumerated() {
                        guard let memberComments = member.leadingComments else {
                            continue
                        }
                        let memberAnnotations = try ValdiTypeScriptAnnotation.extractAnnotations(comments: memberComments,
                                                                                                    fileContent: fileContent)
                        guard !memberAnnotations.isEmpty else {
                            continue
                        }
                        annotatedSymbol.addMemberAnnotations(memberAnnotations, atIndex: idx)
                    }
                }
            }

            return annotatedSymbol
        }
    }
}
