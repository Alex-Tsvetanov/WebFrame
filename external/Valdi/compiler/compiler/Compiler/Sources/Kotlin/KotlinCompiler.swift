//
//  KotlinCompiler.swift
//  Compiler
//
//  Created by Simon Corsin on 12/12/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

import Foundation

class KotlinCompiler {

    struct KotlinJsFile {
        let url: URL
        let relativePath: String
    }

    private var sentKotlinJsFile = false

    private static let kotlinStdLibJsPath = "node_modules/kotlin/kotlin.js"
    private static let expectedOutputPath = "build/classes/kotlin/js/main/ValdiKotlin.js"

    func getKotlinJs(logger: ILogger, gradleDirURL: URL) throws -> KotlinJsFile {
        let currentPath = FileManager.default.currentDirectoryPath
        defer {
            FileManager.default.changeCurrentDirectoryPath(currentPath)
        }
        FileManager.default.changeCurrentDirectoryPath(gradleDirURL.path)

        let processHandle = SyncProcessHandle.usingEnv(logger: logger, command: ["npm", "install", "kotlin"])
        try processHandle.run()

        if !processHandle.exitedSuccesfully {
            throw CompilerError("\(processHandle.stderr.contentAsString)")
        }
        let expectedOutputPathURL = gradleDirURL.appendingPathComponent(KotlinCompiler.kotlinStdLibJsPath)

        if !FileManager.default.fileExists(atPath: expectedOutputPathURL.path) {
            throw CompilerError("Did not find output file at expected location \(expectedOutputPathURL.path)")
        }

        return KotlinJsFile(url: expectedOutputPathURL, relativePath: KotlinCompiler.kotlinStdLibJsPath)
    }

    func compile(logger: ILogger, gradleDirURL: URL) throws -> KotlinJsFile {
        let currentPath = FileManager.default.currentDirectoryPath
        defer {
            FileManager.default.changeCurrentDirectoryPath(currentPath)
        }
        FileManager.default.changeCurrentDirectoryPath(gradleDirURL.path)

        let processHandle = SyncProcessHandle.usingEnv(logger: logger, command: ["gradle", "jsMainClasses"])
        try processHandle.run()

        if !processHandle.exitedSuccesfully {
            throw CompilerError("\(processHandle.stderr.contentAsString)")
        }

        let expectedOutputPathURL = gradleDirURL.appendingPathComponent(KotlinCompiler.expectedOutputPath)

        if !FileManager.default.fileExists(atPath: expectedOutputPathURL.path) {
            throw CompilerError("Did not find output file at expected location \(expectedOutputPathURL.path)")
        }

        return KotlinJsFile(url: expectedOutputPathURL, relativePath: KotlinCompiler.expectedOutputPath)
    }

}
