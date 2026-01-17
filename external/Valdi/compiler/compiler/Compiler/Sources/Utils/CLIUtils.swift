//
//  CLIUtils.swift
//  
//
//  Created by Simon Corsin on 7/29/20.
//

import Foundation

class CLIUtils {

    static func getOutputURLs(commandName: String, baseUrl: URL, out: String?) throws -> URL {
        guard let out = out else {
            throw CompilerError("--out must be provided when using \(commandName) to specify the output file")
        }

        return baseUrl.resolving(path: out)
    }

}
