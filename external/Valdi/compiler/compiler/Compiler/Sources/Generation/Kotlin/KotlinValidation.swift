import Foundation

// Swift doesn't have namespaces, it's customary to use an enum without cases instead
enum KotlinValidation {

    /// These keywords must be avoided when using identifiers for generated code (e.g. for type names, properties, package name components)
    static let kotlinKeywords = [
        "as",
        "break",
        "class",
        "continue",
        "do",
        "else",
        "false",
        "for",
        "fun",
        "if",
        "in",
        "interface",
        "is",
        "null",
        "object",
        "package",
        "return",
        "super",
        "this",
        "throw",
        "true",
        "try",
        "typealias",
        "typeof",
        "val",
        "var",
        "when",
        "while"
    ]

    private static let kotlinKeywordsSet = Set(kotlinKeywords)

    static private let androidRegex = try! NSRegularExpression(pattern: "^[a-z]+(\\.[a-zA-Z_]\\w*)*$")
    static func isValidAndroidTypeName(androidTypeName: String) -> Bool {
        guard androidTypeName.matches(regex: androidRegex) else {
            return false
        }

        let components = androidTypeName.components(separatedBy: ".")
        let noReservedKeywords = components.allSatisfy({ !kotlinKeywordsSet.contains($0.trimmed) })
        return noReservedKeywords
    }
}
