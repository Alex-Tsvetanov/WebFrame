// Copyright © 2024 Snap, Inc. All rights reserved.

import Foundation

struct CompilerFileInputListEntryFile: Codable {
    let file: String
    let content: String?
    let relativeProjectPath: String
}

struct CompilerFileInputListEntry: Codable {
    let moduleName: String
    let modulePath: String
    let moduleContent: String?
    let monitor: Bool?
    let autoDiscover: Bool?
    let files: [CompilerFileInputListEntryFile]
}

struct CompilerFileInputList: Codable {
    let entries: [CompilerFileInputListEntry]
}

extension CompilerFileInputListEntryFile {
    func resolvingVariables(_ variables: [String: String]) throws -> CompilerFileInputListEntryFile {
        return CompilerFileInputListEntryFile(file: try file.resolvingVariables(variables),
                                              content: content,
                                              relativeProjectPath: try relativeProjectPath.resolvingVariables(variables))
    }
}

extension CompilerFileInputListEntry {
    func resolvingVariables(_ variables: [String: String]) throws -> CompilerFileInputListEntry {
        return CompilerFileInputListEntry(moduleName: moduleName,
                                          modulePath: try modulePath.resolvingVariables(variables),
                                          moduleContent: moduleContent,
                                          monitor: monitor,
                                          autoDiscover: autoDiscover,
                                          files: try files.map { try $0.resolvingVariables(variables) })
    }
}

extension CompilerFileInputList {
    func resolvingVariables(_ variables: [String: String]) throws -> CompilerFileInputList {
        return CompilerFileInputList(entries: try self.entries.map {  try $0.resolvingVariables(variables)})
    }
}
