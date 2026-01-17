//
//  CompilerError.swift
//  Compiler
//
//  Created by Simon Corsin on 4/6/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

import Foundation

class CompilerError: LocalizedError, CustomStringConvertible {
    let name: String
    let failedDependenciesURLs: Set<URL>

    init(_ name: String, failedDependenciesURLs: Set<URL> = []) {
        self.name = name
        self.failedDependenciesURLs = failedDependenciesURLs
    }

    convenience init(type: String, message: String, range: NSRange, inDocument document: String, failedDependenciesURLs: Set<URL> = []) {
        let lines = LinesIndexer(str: document)
        let line = lines.lineIndex(atCharacterIndex: range.lowerBound)

        self.init(type: type, message: message, atZeroIndexedLine: line.lineOffset, column: line.characterOffset, inDocument: document, failedDependenciesURLs: failedDependenciesURLs)
    }

    convenience init(type: String, message: String, atZeroIndexedLine lineIndex: Int, column: Int?, inDocument document: String, failedDependenciesURLs: Set<URL> = []) {
        let lines = document.split(separator: "\n", omittingEmptySubsequences: false)
        let minLineToPrint = max(0, lineIndex - 3)
        let maxLineToPrint = min(lines.count - 1, lineIndex + 3)
        let linesToPrint: [String]

        if maxLineToPrint < minLineToPrint {
            linesToPrint = lines.map(String.init)
        } else {
            linesToPrint = (minLineToPrint..<maxLineToPrint+1).flatMap { i -> [String] in
                let indent = "   "
                let emoji = i == lineIndex ? "👉" : " "
                var printLines = [indent + emoji + String(lines[i])]
                if i == lineIndex, let col = column {
                    printLines.append(indent + " " + String(repeating: " ", count: col) + "👆")
                }
                return printLines
            }
        }
        self.init("🚨 \(type): \(message) at line \(lineIndex + 1)\n\n" + linesToPrint.joined(separator: "\n") + "\n", failedDependenciesURLs: failedDependenciesURLs)
    }

    public var errorDescription: String? {
        return name
    }

    var description: String {
        return "<CompilerError: \(name)>"
    }
}
