//
//  Parser.swift
//  Compiler
//
//  Created by Simon Corsin on 12/18/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

import Foundation

class Parser<T: Collection> {

    private let sequence: T
    private var currentIndex: T.Index
    private var endIndex: T.Index

    var isAtEnd: Bool {
        return currentIndex == endIndex
    }

    convenience init(sequence: T) {
        self.init(sequence: sequence, startIndex: sequence.startIndex)
    }

    init(sequence: T, startIndex: T.Index) {
        self.sequence = sequence
        self.currentIndex = startIndex
        self.endIndex = sequence.endIndex
    }

    private func ensureNotAtEnd() throws {
        guard !isAtEnd else {
            throw CompilerError("Unexpectedly reached end of stream")
        }
    }

    private func uncheckedAdvance(distance: Int = 1) {
        currentIndex = sequence.index(currentIndex, offsetBy: distance)
    }

    func peek() throws -> T.Element {
        try ensureNotAtEnd()
        return sequence[currentIndex]
    }

    func advance(distance: Int = 1) throws {
        try ensureNotAtEnd()
        uncheckedAdvance(distance: distance)
    }

    @discardableResult func peekAndAdvance() throws -> T.Element {
        let topValue = try peek()
        uncheckedAdvance()
        return topValue
    }

    func subsequence(length: Int) throws -> T.SubSequence {
        let startIndex = currentIndex

        for _ in 0..<length {
            try advance()
        }

        return sequence[startIndex..<currentIndex]
    }

    func parse(when: (T.Element) -> Bool) throws -> Bool {
        if when(try peek()) {
            uncheckedAdvance()
            return true
        }
        return false
    }

    func mustParse(when: (T.Element) -> Bool) throws {
        guard try parse(when: when) else {
            throw CompilerError("Top element did not match predicate")
        }
    }

}

extension Parser where T.Element: Equatable {

    func parse(element: T.Element) throws -> Bool {
        return try parse { $0 == element }
    }

    func parse<T2: Collection>(subsequence: T2) throws -> Bool where T2.Element == T.Element {
        let indexAtStart = currentIndex
        do {
            for element in subsequence {
                if try !parse(element: element) {
                    currentIndex = indexAtStart
                    return false
                }
            }
            return true
        } catch let error {
            currentIndex = indexAtStart
            throw error
        }
    }

}

// Debugging extension (needs to be in the same file to access `private sequence`)

protocol TextSequenceElement {
    var text: String? { get }
}

extension Parser where T.Element: TextSequenceElement {
    func debugDump() -> String {
        var sequenceStrings: [String] = sequence.indices.map { idx in
            let prefix = idx == currentIndex ? "^" : ""
            let text = sequence[idx].text ?? ""
            return "\(prefix)\(text)"
        }
        if currentIndex == sequence.indices.endIndex {
            sequenceStrings.append("^")
        }
        return sequenceStrings.joined()
    }
}
