//
//  CompilationResult.swift
//  Compiler
//
//  Created by Simon Corsin on 4/7/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

import Foundation

struct CompilationResult {

    var componentPath: ComponentPath

    var originalDocument: ValdiRawDocument

    var templateResult: TemplateCompilerResult

    let classMapping: ResolvedClassMapping

    let userScriptSourceURL: URL?

    let scriptLang: String

    var symbolsToImportsInGeneratedCode = [TypeScriptSymbol]()

}
