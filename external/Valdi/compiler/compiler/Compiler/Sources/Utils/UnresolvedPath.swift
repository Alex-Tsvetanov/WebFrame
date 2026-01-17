//
//  File.swift
//  
//
//  Created by Simon Corsin on 10/18/21.
//

import Foundation

struct UnresolvedPath {
    let baseURL: URL
    let components: [String]
    let variables: [String: String]

    func appendingVariable(key: String, value: String) -> UnresolvedPath {
        var outVariables = self.variables
        outVariables[key] = value

        return UnresolvedPath(baseURL: baseURL, components: components, variables: outVariables)
    }

    func resolve() throws -> URL {
        var current = baseURL
        for component in components {
            let resolvedComponent = try component.resolvingVariables(variables)
            current = current.resolving(path: resolvedComponent, isDirectory: true)
        }

        return current
    }
}
