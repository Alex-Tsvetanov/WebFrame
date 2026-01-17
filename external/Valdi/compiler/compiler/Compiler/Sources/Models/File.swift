//
//  File.swift
//  Compiler
//
//  Created by Simon Corsin on 7/24/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

import Foundation

enum File {
    case url(URL)
    case data(Data)
    case string(String)
    case lazyData(block: () throws -> Data)
}

extension File {

    func readData() throws -> Data {
        switch self {
        case .lazyData(let block):
            let data = try block()
            return data
        case .data(let data):
            return data
        case .string(let str):
            return try str.utf8Data()
        case .url(let url):
            do {
                return try Data(contentsOf: url)
            } catch {
                throw CompilerError("Failed to read Data from \(url): \(error.legibleLocalizedDescription)")
            }
        }
    }

    func readString() throws -> String {
        if case let .string(str) = self {
            return str
        }

        let data = try readData()
        guard let str = String(data: data, encoding: .utf8) else {
            throw CompilerError("Failed to read String from UTF8 Data")
        }
        return str
    }

    func withURL<T>(_ urlClosure: (URL) throws -> T) throws -> T {
        let urlHandle = try getURLHandle()
        return try withExtendedLifetime(urlHandle) {
            return try urlClosure(urlHandle.url)
        }
    }

    func getURLHandle() throws -> URLHandle {
        switch self {
        case .url(let url):
            return DefaultURLHandle(url: url)
        case .data(let data):
            return try disposableURLHandleForData(data: data)
        case .string(let str):
            return try disposableURLHandleForData(data: try str.utf8Data())
        case .lazyData(let block):
            let data = try block()
            return try disposableURLHandleForData(data: data)
        }
    }

    private func disposableURLHandleForData(data: Data) throws -> DisposableURLHandle {
        let fileURL = URL.randomFileURL()
        try data.write(to: fileURL)
        return DisposableURLHandle(url: fileURL)
    }
}

// File is only guaranteed to exist at the URL as long as the handle exists
protocol URLHandle: AnyObject {
    var url: URL { get }
}

private class DefaultURLHandle: URLHandle {
    let url: URL

    init(url: URL) {
        self.url = url
    }
}

private class DisposableURLHandle: URLHandle {
    let url: URL

    init(url: URL) {
        self.url = url
    }

    deinit {
        _ = try? FileManager.default.removeItem(at: url)
    }
}
