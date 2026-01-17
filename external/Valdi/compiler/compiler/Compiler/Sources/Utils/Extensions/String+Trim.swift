//
//  StringUtils.swift
//  Compiler
//
//  Created by Simon Corsin on 4/7/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

import Foundation

private let trimCharacterSet = CharacterSet.whitespacesAndNewlines

extension Substring {

    var trimmed: Substring {
        let scalars = self.unicodeScalars

        var newStartIndex = scalars.startIndex
        var newEndIndex = scalars.endIndex

        while newStartIndex < newEndIndex && trimCharacterSet.contains(scalars[newStartIndex]) {
            newStartIndex = scalars.index(newStartIndex, offsetBy: 1)
        }
        while newEndIndex > newStartIndex {
            let maybeNewEndIndex = scalars.index(newEndIndex, offsetBy: -1)
            if trimCharacterSet.contains(scalars[maybeNewEndIndex]) {
                newEndIndex = maybeNewEndIndex
            } else {
                break
            }
        }

        return Substring(scalars[newStartIndex..<newEndIndex])
    }

    var unquote: Substring {
        if (hasPrefix("'") && hasSuffix("'")) || (hasPrefix("\"") && hasSuffix("\"")) {
            return self[index(startIndex, offsetBy: 1)..<index(endIndex, offsetBy: -1)]
        }
        return self
    }

    var pascalCased: String {
        return String(self).pascalCased
    }

    var camelCased: String {
        return String(self).camelCased
    }

}

extension String {

    var trimmed: String {
        return self.trimmingCharacters(in: trimCharacterSet)
    }

    var xmlEscaped: String {
        return self
            .replacingOccurrences(of: "&", with: "&amp;")
            .replacingOccurrences(of: ">", with: "&gt;")
            .replacingOccurrences(of: "<", with: "&lt;")
    }

    var jsonEscaped: String {
        return self
            .replacingOccurrences(of: "\\", with: "\\\\")
            .replacingOccurrences(of: "\n", with: "\\n")
            .replacingOccurrences(of: "\t", with: "\\t")
            .replacingOccurrences(of: "'", with: "\\'")
            .replacingOccurrences(of: "\"", with: "\\\"")
    }

    var removingLastCharacter: String {
        let lastIndex = self.index(endIndex, offsetBy: -1)

        return String(self[startIndex..<lastIndex])
    }

    var asValdiAttributeKey: String {
        var out = String.UnicodeScalarView()
        var shouldUpper = false

        for currentCharacter in self.unicodeScalars {
            if currentCharacter == "-" || currentCharacter == "_" {
                if !out.isEmpty {
                    shouldUpper = true
                }
                continue
            }

            var c = currentCharacter

            if shouldUpper {
                shouldUpper = false
                if c >= "a" && c <= "z" {
                    c = Unicode.Scalar(c.value - 32)!
                }
            }

            if (c >= "a" && c <= "z") || (c >= "A" && c <= "Z") || (c >= "0" && c <= "9") || (c == ":") {
                out.append(c)
            }
        }

        return String(out)
    }

    var unquote: Substring {
        if (hasPrefix("'") && hasSuffix("'")) || (hasPrefix("\"") && hasSuffix("\"")) {
            return self[index(startIndex, offsetBy: 1)..<index(endIndex, offsetBy: -1)]
        }
        return self[startIndex..<endIndex]
    }

    var camelCased: String {
        var str = self.asValdiAttributeKey
        if !str.isEmpty {
            str = str[0..<1].lowercased() + str[1..<str.count]
        }
        return str
    }

    var pascalCased: String {
        var str = self.asValdiAttributeKey
        if !str.isEmpty {
            str = str[0..<1].uppercased() + str[1..<str.count]
        }
        return str
    }

    var snakeCased: String {
        let converted = self.asValdiAttributeKey
        if converted.isEmpty {
            return self
        }

        var allowSnakeInsertion = false

        var snakeCased = ""
        for char in converted {
            if char.isUppercase {
                if allowSnakeInsertion {
                    allowSnakeInsertion = false
                    snakeCased.append("_")
                }
                snakeCased.append(String(char).lowercased())
            } else {
                allowSnakeInsertion = true
                snakeCased.append(char)
            }
        }
        return snakeCased
    }

    subscript(range: Range<Int>) -> Substring {
        let fromIndex = index(startIndex, offsetBy: range.lowerBound)
        let toIndex = index(fromIndex, offsetBy: range.count)

        return self[fromIndex..<toIndex]
    }

    subscript(index: Int) -> Character {
        return self[self.index(startIndex, offsetBy: index)]
    }

    func removingCharacters(in set: CharacterSet) -> String {
        let filtered = unicodeScalars.lazy.filter { !set.contains($0) }
        return String(String.UnicodeScalarView(filtered))
    }

    var removingFirstCharacter: String {
        return String(self[self.index(self.startIndex, offsetBy: 1)...])
    }

    func split(at: Character) -> (String, String) {
        let kv = self.split(separator: at, maxSplits: 1, omittingEmptySubsequences: true)
        if kv.count != 2 {
            return (String(kv[0]), "")
        }
        return (String(kv[0]), String(kv[1]))
    }

    func trim(after: Character) -> String {
        let (trimmed, _) = self.split(at: after)
        return trimmed
    }

    func removing(suffix: String) -> String {
        if hasSuffix(suffix) {
            return String(self[self.startIndex..<self.index(self.startIndex, offsetBy: self.count - suffix.count)])
        } else {
            return self
        }
    }

    func removing(suffixes: [String]) -> String {
        var current = self

        for suffix in suffixes {
            current = current.removing(suffix: suffix)
        }

        return current
    }
}

private let lowercaseLettersSet = CharacterSet.lowercaseLetters

extension Character {

    var isUppercase: Bool {
        return !String(self).trimmingCharacters(in: lowercaseLettersSet).isEmpty
    }

}

extension StringProtocol {

    var escapingSpaces: String {
        return self.replacingOccurrences(of: " ", with: "\\ ")
    }

    func utf8Data() throws -> Data {
        guard let data = self.data(using: .utf8) else {
            throw CompilerError("Could not get UTF8 Data from string")
        }
        return data
    }

}
