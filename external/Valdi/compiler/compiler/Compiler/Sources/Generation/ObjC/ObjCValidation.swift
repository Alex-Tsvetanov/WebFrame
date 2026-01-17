import Foundation

// Swift doesn't have namespaces, it's customary to use an enum without cases instead
enum ObjCValidation {

    /// These keywords must be avoided when using identifiers for generated code (e.g. type names, properties)
    ///
    /// https://www.oreilly.com/library/view/learning-objective-c-20/9780133047462/app01.html
    /// https://www.binpress.com/objective-c-reserved-keywords/
    static let objcKeywords = [

        // additional
        "description",

        // hard
        "asm",
        "auto",
        "break",
        "case",
        "char",
        "const",
        "continue",
        "default",
        "do",
        "double",
        "else",
        "enum",
        "extern",
        "float",
        "for",
        "goto",
        "if",
        "inline",
        "int",
        "long",
        "register",
        "restrict",
        "return",
        "short",
        "signed",
        "sizeof",
        "static",
        "struct",
        "switch",
        "typedef",
        "union",
        "unsigned",
        "void",
        "volatile",
        "_Bool",
        "_Complex",
        "_Imaginary",
        "__block",

        // soft
        "BOOL",
        "Class",
        "bycopy",
        "byref",
        "id",
        "IMP",
        "in",
        "In",
        "inout",
        "nil",
        "Nil",
        "NO",
        "NULL",
        "oneway",
        "out",
        "Protocol",
        "SEL",
        "self",
        "super",
        "YES",
        "@interface",
        "@end",
        "@implementation",
        "@protocol",
        "@class",
        "@public",
        "@protected",
        "@private",
        "@property",
        "@try",
        "@throw",
        "@catch",
        "@finally",
        "@synthesize",
        "@dynamic",
        "@selector",
        "atomic",
        "nonatomic",
        "retain",
        "_cmd",
        "__autoreleasing",
        "__strong",
        "__weak",
        "__unsafe_unretained"
    ]

    static private let objcIdentifierRegex = try! NSRegularExpression(pattern: "^[a-zA-Z_]+([a-zA-Z_0-9])*$")

    static func isValidIOSTypeName(iosTypeName: String) -> Bool {
        return iosTypeName.matches(regex: objcIdentifierRegex)
    }
}
