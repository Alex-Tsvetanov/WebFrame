//
//  TypeScriptCompilerTSServerDriver.swift
//  
//
//  Created by saniul on 31/12/2019.
//

import Foundation

protocol TypeScriptCompilerDriver {
    func initializeWorkspace() -> Promise<Void>

    func registerInMemoryFile(filePath: String, fileContent: String) -> Promise<Void>

    func registerDiskFile(filePath: String, absoluteFileURL: URL) -> Promise<Void>

    func openFile(filePath: String) -> Promise<TS.OpenResponseBody>

    func getErrors(filePath: String) -> Promise<GetErrorsResult>

    func saveFile(filePath: String) -> Promise<[TS.EmittedFile]>

    func compileNative(filePaths: [String]) -> Promise<[TS.EmittedFile]>

    func dumpSymbolsWithComments(filePath: String) -> Promise<TS.DumpSymbolsWithCommentsResponseBody>

    func dumpInterface(filePath: String, position: Int) -> Promise<TS.DumpInterfaceResponseBody>

    func dumpEnum(filePath: String, position: Int) -> Promise<TS.DumpEnumResponseBody>

    func dumpFunction(filePath: String, position: Int) -> Promise<TS.DumpFunctionResponseBody>
}
