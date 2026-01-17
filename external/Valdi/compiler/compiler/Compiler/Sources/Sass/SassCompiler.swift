//
//  Sass.swift
//  Compiler
//
//  Created by Brandon Francis on 8/20/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

import Foundation

/**
 Compiles sass into css.
 https://sass-lang.com/
 */
class SassCompiler {

    public func compile(file: File, importBaseDirectoryURL: URL) throws -> SassResult {
        let input = try file.readString()
        if input.isEmpty {
            return SassResult(output: "", includedFilePaths: [])
        }

        return try LibSassCompiler.compile(inputString: input, includePath: importBaseDirectoryURL.path)
    }
}
