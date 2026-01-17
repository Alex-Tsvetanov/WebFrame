//
//  UserScriptManager.swift
//  Compiler
//
//  Created by saniul on 16/06/2019.
//

import Foundation

struct UserScript {
    let content: File
    let lang: String
    let lineOffset: Int
}

struct UserScriptResult {
    let userScript: UserScript
    let emitScriptCompilationItem: CompilationItem?
}

final class UserScriptManager {

    private var registeredUserScripts = Synchronized<[URL: UserScript]>(data: [:])

    func findUserScript(documentSourceURL: URL) -> UserScript? {
        return registeredUserScripts.data { $0[documentSourceURL] }
    }

    func findUserScriptURL(documentSourceURL: URL) -> URL? {
        guard let userScript = findUserScript(documentSourceURL: documentSourceURL) else {
            return nil
        }

        switch userScript.content {
        case .url(let url):
            return url
        case .data, .lazyData, .string:
            return documentSourceURL.appendingPathExtension(userScript.lang)
        }
    }

    func processUserScript(documentItem: CompilationItem, rawDocument: ValdiRawDocument) throws -> UserScriptResult? {
        let inputURL = documentItem.sourceURL
        let baseURL = inputURL.deletingLastPathComponent()

        var emitScriptCompilationItem: CompilationItem?

        guard let userScript = try makeUserScript(baseURL: baseURL, rawDocument: rawDocument) else {
            return nil
        }

        let relativeProjectPath: String
        var shouldEmitItem = false
        let url: URL
        switch userScript.content {
        case .url(let urlScript):
            url = urlScript
            relativeProjectPath = documentItem.bundleInfo.relativeProjectPath(forItemURL: urlScript)
        case .data, .lazyData, .string:
            url = inputURL.appendingPathExtension(userScript.lang)
            relativeProjectPath = documentItem.relativeProjectPath + ".\(userScript.lang)"
            shouldEmitItem = true
        }

        let kind: CompilationItem.Kind

        switch userScript.lang {
        case FileExtensions.typescriptXml:
            fallthrough
        case FileExtensions.typescript:
            kind = .typeScript(userScript.content, url)
        case FileExtensions.javascript:
            kind = .javaScript(JavaScriptFile(file: userScript.content, relativePath: relativeProjectPath))
        case FileExtensions.kotlin:
            kind = .kotlin(userScript.content, url)
        default:
            throw CompilerError(type: "Script error", message: "Unsupported script lang '\(userScript.lang)'", atZeroIndexedLine: userScript.lineOffset, column: nil, inDocument: rawDocument.content)
        }

        if shouldEmitItem {
            // We emit a new compilation item for the user generated code.
            emitScriptCompilationItem = CompilationItem(sourceURL: url, relativeProjectPath: relativeProjectPath, kind: kind, bundleInfo: documentItem.bundleInfo, platform: documentItem.platform, outputTarget: .all)
        }

        registeredUserScripts.data { $0[inputURL] = userScript }

        return UserScriptResult(userScript: userScript,
                                  emitScriptCompilationItem: emitScriptCompilationItem)
    }

    private func makeUserScript(baseURL: URL, rawDocument: ValdiRawDocument) throws -> UserScript? {
        var resolvedLang: String?
        var scriptFiles = [File]()
        var lineOffset = 0
        for script in rawDocument.scripts {
            lineOffset = script.lineOffset ?? 0
            if let content = script.content {
                scriptFiles.append(.data(try content.utf8Data()))
            }
            if let source = script.source {
                let url = baseURL.resolving(path: source)
                if resolvedLang == nil {
                    resolvedLang = url.pathExtension
                }
                scriptFiles.append(.url(url))
            }
            if let lang = script.lang {
                if resolvedLang != nil && resolvedLang != lang {
                    throw CompilerError(type: "Script error", message: "Conflicting lang '\(resolvedLang!)' and '\(lang)'", atZeroIndexedLine: script.lineOffset ?? 0, column: nil, inDocument: rawDocument.content)
                }
                resolvedLang = lang
            }
        }

        if scriptFiles.isEmpty {
            return nil
        }

        let resolvedContent: File

        if scriptFiles.count == 1 {
            resolvedContent = scriptFiles[0]
        } else {
            var newData = Data()
            try scriptFiles.forEach { newData.append(try $0.readData()) }
            resolvedContent = .data(newData)
        }

        return UserScript(content: resolvedContent, lang: resolvedLang ?? FileExtensions.typescript, lineOffset: lineOffset)
    }
}
