import Foundation

class SwiftEmittedIdentifiers: EmittedIdentifiers {
    var registerFunctions: [String: String] = [:]
    var initializationString: CodeWriter {
        let out = CodeWriter()
        out.appendBody("        [\n")
        for identifier in identifiers {
            let registerFunc = registerFunctions[identifier] ?? "nil"
            out.appendBody("            ValdiMarshallableIdentifier(name:\"\(identifier)\", registerFunc: \(registerFunc)),\n")
        }
        out.appendBody("        ]\n")
        return out
    }

    public func setRegisterFunction(forIdentifier identifier: String, registerFunction: String) {
        registerFunctions[identifier] = registerFunction
    }
}
