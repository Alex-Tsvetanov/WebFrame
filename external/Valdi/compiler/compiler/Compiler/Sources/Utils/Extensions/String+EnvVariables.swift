//
//  String+EnvVariables.swift
//  Compiler
//
//  Created by Simon Corsin on 2/2/19.
//  Copyright © 2019 Snap Inc. All rights reserved.
//

import Foundation

private let envVarRegex = try! NSRegularExpression(pattern: "\\$([a-zA-Z0-9]+[a-zA-Z0-9_]*)")

extension String {

    private func replacingVariables(regex: NSRegularExpression, onReplace: (String) throws -> String) rethrows -> String {
        var current = self

        while true {
            let utf16 = current.utf16
            guard let match = regex.firstMatch(in: current, options: [], range: NSRange(location: 0, length: utf16.count)) else {
                return current
            }

            let range = match.range(at: 1)
            let envVariable = current.substring(with: range)!
            let replacement = try onReplace(envVariable)
            current = current.replacing(range: match.range, with: replacement) ?? ""
        }
    }

    func resolvingVariables(_ variables: [String: String]) throws -> String {
        return try replacingVariables(regex: envVarRegex) { key in
            guard let value = variables[key] else {
                throw CompilerError("Could not resolve variable \"\(key)\"")
            }
            return value
        }
    }

}
