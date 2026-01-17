import Foundation

class TypeScriptAnnotatedSymbol {
    let symbol: TS.DumpedSymbolWithComments

    // All the annotations found in the leading comments
    private(set) var annotations: [ValdiTypeScriptAnnotation] = []

    // All the annotations found on members, by member index
    private(set) var memberAnnotations: [Int: [ValdiTypeScriptAnnotation]] = [:]

    init(symbol: TS.DumpedSymbolWithComments) {
        self.symbol = symbol
    }

    func addAnnotations(_ annotations: [ValdiTypeScriptAnnotation]) {
        self.annotations.append(contentsOf: annotations)
    }

    func addMemberAnnotations(_ memberAnnotations: [ValdiTypeScriptAnnotation], atIndex idx: Int) {
        self.memberAnnotations[idx, default: []] += memberAnnotations
    }

    func getAnnotation(_ annotation: String) -> ValdiTypeScriptAnnotation? {
        return annotations.first(where: { $0.name == annotation })
    }

    func mergedCommentsWithoutAnnotations() -> String {
        TypeScriptAnnotatedSymbol.mergedCommentsWithoutAnnotations(fullComments: symbol.leadingComments?.text ?? "", annotations: self.annotations)
    }

    static func mergedCommentsWithoutAnnotations(fullComments: String, annotations: [ValdiTypeScriptAnnotation]) -> String {
        var commentsString = fullComments
        for annotation in annotations {
            commentsString = commentsString.replacingOccurrences(of: annotation.content, with: "")
        }

        let clean = cleanCommentString(commentsString)
        return clean
    }

    static private let cleanCommentsCharacterSet = CharacterSet.whitespacesAndNewlines.union(CharacterSet(charactersIn: "*/"))
    static func cleanCommentString(_ comment: String) -> String {
        let lines = comment
            .components(separatedBy: CharacterSet.newlines)

        let clean = lines.map { $0.trimmingCharacters(in: cleanCommentsCharacterSet) }
          .joined(separator: "\n")
          .trimmed

        return clean
    }

}
