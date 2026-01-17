import Foundation

// Swift doesn't have namespaces, it's customary to use an enum without cases instead
enum SwiftValidation {

    /// These keywords must be avoided when using identifiers for generated code (e.g. type names, properties)
    ///
    /// https://www.geeksforgeeks.org/swift-keywords/
    ///
    
    // Keywords used in the declaration
    static let swiftKeywords = [
        "associatedtype",
        "class",
        "deinit",
        "enum",
        "extension",
        "fileprivate",
        "func",
        "import",
        "init",
        "inout",
        "internal",
        "let",
        "open",
        "operator",
        "private",
        "precedencegroup",
        "protocol",
        "public",
        "rethrows",
        "static",
        "struct",
        "subscript",
        "typealias",
        "var",
        
        // Keywords used in statements
        "break",
        "case",
        "catch",
        "continue",
        "default",
        "defer",
        "do",
        "else",
        "fallthrough",
        "for",
        "guard",
        "if",
        "in",
        "repeat",
        "return",
        "throw",
        "switch",
        "where",
        "while",
        
        // Keywords used in expression and type
        "Any",
        "as",
        "catch",
        "false",
        "is",
        "nil",
        "rethrows",
        "self",
        "Self",
        "super",
        "throw",
        "throws",
        "true",
        "try",
        
        // Keywords used in the specific context
        "associativity",
        "convenience",
        "didSet",
        "dynamic",
        "final",
        "get",
        "indirect",
        "infix",
        "lazy",
        "left",
        "mutating",
        "none",
        "nonmutating",
        "optional",
        "override",
        "postfix",
        "precedence",
        "prefix",
        "Protocol",
        "required",
        "right",
        "set",
        "some",
        "Type",
        "unowned",
        "weak",
        "willSet",
    ]

    static private let objcIdentifierRegex = try! NSRegularExpression(pattern: "^[a-zA-Z_]+([a-zA-Z_0-9])*$")

    static func isValidIOSTypeName(iosTypeName: String) -> Bool {
        return iosTypeName.matches(regex: objcIdentifierRegex)
    }
}
