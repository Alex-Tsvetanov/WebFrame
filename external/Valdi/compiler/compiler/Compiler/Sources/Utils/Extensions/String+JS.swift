//
//  String+JS.swift
//  Compiler
//
//  Created by Simon Corsin on 5/3/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

import Foundation

extension String {

    var indented: String {
        return indented(indentPattern: "  ")
    }

    func indented(indentPattern: String) -> String {
        // not completely correct but helps debugging
        let lines = self.split(separator: "\n", maxSplits: Int.max, omittingEmptySubsequences: false)

        var indentCount = 0

        var outLines = [String]()
        var previousWasEmptyLine = true
        for line in lines {
            let trimmedLine = line.trimmed

            if trimmedLine.isEmpty {
                if previousWasEmptyLine {
                    continue
                }
                outLines.append("\n")
                previousWasEmptyLine = true
                continue
            }
            previousWasEmptyLine = false

            var newIndent = indentCount

            for (index, c) in trimmedLine.enumerated() {
                if c == "{" {
                    newIndent += 1
                } else if c == "}" {
                    newIndent -= 1
                    if index == 0 {
                        indentCount -= 1
                    }
                }
            }

            // indentCount may be < 0 if the syntax is invalid -- we still shouldn't crash:
            indentCount = max(0, indentCount)

            let indent = String(repeating: indentPattern, count: indentCount)

            indentCount = newIndent

            outLines.append(indent + trimmedLine + "\n")
        }

        return outLines.joined()
    }

    var strippingSingleLineComments: String {
        return self.split(separator: "\n")
            .filter { !$0.trimmed.starts(with: "//") }
            .joined(separator: "\n")
    }

    var asJsExpression: String {
        if isBool || isNumber {
            return self
        }
        return "'" + self + "'"
    }

    var isBool: Bool {
        return self == "true" || self == "false"
    }

    private static var nonDecimalSet: CharacterSet = CharacterSet.decimalDigits.inverted

    var isNumber: Bool {
        return self.rangeOfCharacter(from: String.nonDecimalSet) == nil
    }
}
