//
//  CSSCompiler.swift
//  Compiler
//
//  Created by Simon Corsin on 4/7/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

import Foundation

class StyleSheetCompiler {

    private let logger: ILogger
    private let content: String
    private let baseURL: URL
    private let relativeProjectPath: String

    private(set) var filesImported: Set<URL> = []
    private var sassCompiler: SassCompiler = SassCompiler()

    init(logger: ILogger, content: String, baseURL: URL, relativeProjectPath: String) {
        self.logger = logger
        self.content = content
        self.baseURL = baseURL
        self.relativeProjectPath = relativeProjectPath
    }

    private func importCssFile(url: URL, tree: StyleTree) throws {
        guard !filesImported.contains(url) else {
            throw CompilerError("Cyclic import detected")
        }

        filesImported.insert(url)
        let cssContent = try String(contentsOf: url)
        return try processCss(cssContent: cssContent, baseURL: url, tree: tree)
    }

    private func processCss(cssContent: String, baseURL: URL, tree: StyleTree) throws {
        logger.verbose("Processing CSS for \(relativeProjectPath)")
        var importFilenames = [String]()
        try tree.add(logger: logger, cssString: cssContent, filename: baseURL.path) { filename in
            importFilenames.append(filename)
        }

        for filename in importFilenames {
            let fileURL = baseURL.resolving(path: filename)
            try importCssFile(url: fileURL, tree: tree)
        }
    }

    private func compileSass(sassContent: String, baseURL: URL) throws -> String {
        logger.verbose("Compiling SASS for \(relativeProjectPath)")
        guard let data = sassContent.data(using: .utf8) else {
            throw CompilerError("Error converting style content into data")
        }

        // The sass compiler will automatically resolve .scss imports, but .css imports will still exist
        let result = try sassCompiler.compile(file: .data(data), importBaseDirectoryURL: baseURL)
        for includedFilePath in result.includedFilePaths {
            filesImported.insert(baseURL.resolving(path: includedFilePath))
        }

        return result.output
    }

    func compile() throws -> StyleSheetCompilerResult {
        let tree = StyleTree()

        self.filesImported = []

        // Always compile as sass first, then resolve css imports that still exist after compilation
        let cssContent = try compileSass(sassContent: content, baseURL: baseURL)
        try processCss(cssContent: cssContent, baseURL: baseURL, tree: tree)

        return StyleSheetCompilerResult(rootStyleNode: tree.root.toProto(logger: logger))
    }

}
