//
//  TypeScriptStringsModuleGenerator.swift
//  
//
//  Created by Simon Corsin on 3/23/20.
//

import Foundation
import Yams

struct TypeScriptStringsModuleResult {
    let jsFile: File
    let tsDefinitionFile: File
}

private struct LocalizableStringParam {
    enum Specifier {
        case string
        case int
    }

    let name: String
    let specifier: Specifier
}

private struct LocalizableString {
    let key: String
    let params: [LocalizableStringParam]
    let message: String
    let example: String?
}

private class ParsedLocalizableStrings {
    var strings: [LocalizableString]

    init(strings: [LocalizableString]) {
        self.strings = strings
    }
}

struct LocalizableStringsContent {
    let relativePath: RelativePath
    let file: File
}

class TypeScriptStringsModuleGenerator {

    private let moduleName: String

    private let paramRegex = try! NSRegularExpression(pattern: "(\\{([\\w_]+)(%.*?)?\\})")
    private let stringFiles: [LocalizableStringsContent]
    private let defaultStringFileIndex: Int
    private let emitInlineTranslations: Bool

    init(stringFiles: [LocalizableStringsContent], defaultStringFileIndex: Int, moduleName: String, emitInlineTranslations: Bool) {
        self.stringFiles = stringFiles
        self.defaultStringFileIndex = defaultStringFileIndex
        self.moduleName = moduleName
        self.emitInlineTranslations = emitInlineTranslations
    }

    static func outputPath(bundleName: String, ext: String) -> String {
        return "\(bundleName)/src/Strings.\(ext)"
    }

    private func parseLocalizableString(key: String, message: String, example: String?, comment: String?) throws -> LocalizableString {
        let matches = paramRegex.matches(in: message, options: [], range: message.nsrange)

        let nsMessage = message as NSString

        var params = [LocalizableStringParam]()
        for match in matches {
            let paramName: String

            let paramNameRange = match.range(at: 2)
            if paramNameRange.location != NSNotFound {
                paramName = nsMessage.substring(with: paramNameRange)
            } else {
                paramName = "unnamed\(params.count + 1)"
            }

            let specifier: LocalizableStringParam.Specifier

            let specifierRange = match.range(at: 3)
            if specifierRange.location != NSNotFound {
                let specifierString = nsMessage.substring(with: specifierRange)
                switch specifierString {
                case "%d":
                    specifier = .int
                case "%s":
                    specifier = .string
                default:
                    throw CompilerError("Unrecognized specifier '\(specifierString)' in message '\(message)' at key '\(key)'")
                }
            } else {
                specifier = .string
            }

            params.append(LocalizableStringParam(name: paramName, specifier: specifier))
        }

        return LocalizableString(key: key, params: params, message: message, example: example)
    }

    private func extractStringsFromJSON(json: Any) throws -> ParsedLocalizableStrings {
        guard let rootDict = json as? [String: Any] else {
            throw CompilerError("Invalid strings JSON format")
        }

        let localizableStrings = try rootDict.compactMap { keyAndValue -> LocalizableString? in
            let key = keyAndValue.key
            let untypedRecord = keyAndValue.value

            // Snap-specific: a "l10nsvc": "sourcefile" key/value pair is used as an annotation required by the Snap Localization Service
            // We ignore it when processing the strings.json file
            guard key != "l10nsvc" else {
                return nil
            }

            guard let record = untypedRecord as? [String: Any] else {
                throw CompilerError("Invalid record for key \(key). Expected a dictionary.")
            }

            guard let message = record["defaultMessage"] as? String else {
                throw CompilerError("Invalid 'message' for key \(key)")
            }
            let example = record["example"] as? String
            let comment = record["comment"] as? String
            return try parseLocalizableString(key: key, message: message, example: example, comment: comment)
        }
        return ParsedLocalizableStrings(strings: localizableStrings)
    }

    private func parseStrings(file: File) throws -> ParsedLocalizableStrings {
        let fileContent = try file.readData()
        let json = try JSONSerialization.jsonObject(with: fileContent)
        return try extractStringsFromJSON(json: json)
    }

    private func resolveLanguage(fromPath: RelativePath) throws -> String {
        let filename = fromPath.components.last ?? ""

        guard filename.hasPrefix(Files.stringsJSONPrefix) && filename.hasSuffix(Files.stringsJSONSuffix) else {
            throw CompilerError("File '\(filename)' is not a string JSON file")
        }

        let startIndex = Files.stringsJSONPrefix.endIndex
        let endIndex = filename.index(filename.endIndex, offsetBy: -Files.stringsJSONSuffix.count)

        return String(filename[startIndex..<endIndex])
    }

    private func resolveAvailableLanguagesAndFilePaths() throws -> [(String, String)] {
        let defaultFile = self.stringFiles[self.defaultStringFileIndex]

        for file in self.stringFiles where file.relativePath != defaultFile.relativePath {
            // We don't do any fancy processing with the file here, but we parse it
            // to validate that it is at least correct
            let _ = try parseStrings(file: file.file)
        }

        return try self.stringFiles.map {
            let language = try resolveLanguage(fromPath: $0.relativePath)
            return (language, $0.relativePath.description)
        }.sorted(by: { $0.0 < $1.0 })
    }

    func generate() throws -> TypeScriptStringsModuleResult {
        let localizableStrings = try parseStrings(file: self.stringFiles[self.defaultStringFileIndex].file)
        localizableStrings.strings.sort { (left, right) -> Bool in
            return left.key < right.key
        }

        let jsFile = CodeWriter()
        let tsDefFile = CodeWriter()

        let stringKeys = CodeWriter()
        let parameterTypes = CodeWriter()

        jsFile.appendBody("exports.default = require('valdi_core/src/LocalizableStrings').")
        if emitInlineTranslations {
            jsFile.appendBody("buildInlineModule('\(moduleName)', [")
            jsFile.appendBody(stringKeys)
            jsFile.appendBody("], [")
            jsFile.appendBody(parameterTypes)
            jsFile.appendBody("], [")

            let availableLanguagesAndFilePaths = try resolveAvailableLanguagesAndFilePaths()

            jsFile.appendBody(availableLanguagesAndFilePaths.map { "\"\($0.0)\"" }.joined(separator: ", "))
            jsFile.appendBody("], [")
            jsFile.appendBody(availableLanguagesAndFilePaths.map { "\"\($0.1)\"" }.joined(separator: ", "))
            jsFile.appendBody("]);\n")
        } else {
            jsFile.appendBody("buildExternalModule('\(moduleName)', [")
            jsFile.appendBody(stringKeys)
            jsFile.appendBody("], [")
            jsFile.appendBody(parameterTypes)
            jsFile.appendBody("]);\n")
        }

        tsDefFile.appendBody("export interface Strings {\n")

        var isFirst = true

        for localizableString in localizableStrings.strings {
            if !isFirst {
                stringKeys.appendBody(", ")
                parameterTypes.appendBody(", ")
                tsDefFile.appendBody("\n")
            }
            isFirst = false

            stringKeys.appendBody("'\(localizableString.key)'")

            var functionComment = "Returns the localizable string for the key '\(localizableString.key)'"
            if let example = localizableString.example {
                functionComment += "\nExample: "
                functionComment += example
            }

            var resolvedFunctionComment = FileHeaderCommentGenerator.generateMultilineComment(comment: functionComment)
            // Indent comment by 2 spaces
            resolvedFunctionComment = resolvedFunctionComment.split(separator: "\n").map { "  \($0)" }.joined(separator: "\n")

            tsDefFile.appendBody(resolvedFunctionComment)
            tsDefFile.appendBody("\n  \(localizableString.key.camelCased)(")

            var tsParamDecls: [String] = []
            var jsParamTypes: [String] = []

            for param in localizableString.params {
                let tsParamType: String
                let jsParamType: String

                switch param.specifier {
                case .string:
                    tsParamType = "string"
                    jsParamType = "0"
                case .int:
                    tsParamType = "number"
                    jsParamType = "1"
                }

                tsParamDecls.append("\(param.name.camelCased): \(tsParamType)")
                jsParamTypes.append(jsParamType)
            }

            tsDefFile.appendBody(tsParamDecls.joined(separator: ", "))
            tsDefFile.appendBody("): string,\n")

            if jsParamTypes.isEmpty {
                parameterTypes.appendBody("undefined")
            } else {
                parameterTypes.appendBody("[")
                parameterTypes.appendBody(jsParamTypes.joined(separator: ", "))
                parameterTypes.appendBody("]")
            }
        }

        tsDefFile.appendBody("}\n")
        tsDefFile.appendBody("\ndeclare const strings: Strings;\n")
        tsDefFile.appendBody("export default strings;\n")

        return TypeScriptStringsModuleResult(jsFile: .data(try jsFile.data()), tsDefinitionFile: .data(try tsDefFile.data()))
    }

}
