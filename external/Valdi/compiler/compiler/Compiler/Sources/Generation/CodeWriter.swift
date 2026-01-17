//
//  CodeWriter.swift
//  Compiler
//
//  Created by Simon Corsin on 4/12/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

import Foundation

protocol CodeWriterContent {

    var content: String { get }

}

extension String: CodeWriterContent {

    var content: String { return self }

}

class TemplateContext {
    fileprivate var replacements = [String: CodeWriterContent]()

    func set(arg: String, value: CodeWriterContent) {
        self.replacements[arg] = value
    }

    subscript(arg: String) -> CodeWriterContent? {
        get {
            return replacements[arg]
        }
        set(newValue) {
            replacements[arg] = newValue
        }
    }
}

class TemplateContent: CodeWriterContent {

    let context: TemplateContext
    let template: String

    init(template: String) {
        self.context = TemplateContext()
        self.template = template
    }

    var content: String {
        var out = template

        for replacement in context.replacements {
            let replacementContent = replacement.value.content

            let key = "$\(replacement.key)"
            out = out.replacingOccurrences(of: key, with: replacementContent)
        }
        return out
    }
}

class JoinedContent: CodeWriterContent {

    private var items: [CodeWriterContent]
    private let separator: String

    init(separator: String) {
        self.items = []
        self.separator = separator
    }

    init(items: [CodeWriterContent], separator: String) {
        self.items = items
        self.separator = separator
    }

    func append(content: CodeWriterContent) {
        self.items.append(content)
    }

    var content: String {
        return self.items.map { $0.content }.joined(separator: separator)
    }
}

class CodeWriter: CodeWriterContent {

    var content: String {
        var output = (headers + body).map { $0.content }.joined()
        if !output.isEmpty {
            output += nonEmptyTerminator
        }

        return output
    }

    /**
     A terminator string to add if the content was not empty
     */
    var nonEmptyTerminator: String = ""

    private var headers = [CodeWriterContent]()
    private var body = [CodeWriterContent]()

    func appendHeader(_ str: CodeWriterContent) {
        headers.append(str)
    }

    func prependHeader(_ str: CodeWriterContent) {
        headers.insert(str, at: 0)
    }

    func prependBody(_ str: CodeWriterContent) {
        body.insert(str, at: 0)
    }

    func appendBody(_ str: CodeWriterContent) {
        body.append(str)
    }

    func data() throws -> Data {
        let content = self.content
        guard let data = content.data(using: .utf8) else {
            throw CompilerError("Failed to make data out of string")
        }
        return data
    }

}
