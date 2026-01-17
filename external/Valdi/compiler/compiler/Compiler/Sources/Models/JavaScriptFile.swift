//
//  JavaScriptFile.swift
//
//
//  Created by Simon Corsin on 1/24/20.
//

import Foundation

struct JavaScriptFile {
    let file: File
    let relativePath: String

    init(file: File, relativePath: String) {
        self.file = file
        self.relativePath = relativePath
    }

    func with(file: File) -> JavaScriptFile {
        return JavaScriptFile(file: file, relativePath: self.relativePath)
    }
}
