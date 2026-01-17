//
//  ValdiFileManager.swift
//
//
//  Created by Simon Corsin on 4/2/21.
//

import Foundation

class ValdiFileManager {

    struct SaveOptions: OptionSet {
        let rawValue: Int

        static let overwriteOnlyIfDifferent = SaveOptions(rawValue: 1 << 0)

        static let defaultOptions: SaveOptions = []
    }

    private let lock = DispatchSemaphore.newLock()

    private let bazel: Bool

    private let trashDirectoryURL = URL(fileURLWithPath: NSTemporaryDirectory())
        .appendingPathComponent("valdi-trash", isDirectory: true)
        .appendingPathComponent(UUID().uuidString, isDirectory: true)
    private var directoryCheckCache = Set<String>()

    init(bazel: Bool) throws {
        self.bazel = bazel
        if FileManager.default.fileExists(atPath: trashDirectoryURL.path) {
            try FileManager.default.removeItem(at: trashDirectoryURL)
        }
        try doCreateDirectory(atAbsoluteURL: trashDirectoryURL)
    }

    func save(file: File, to url: URL, options: SaveOptions = .defaultOptions) throws {
        switch file {
        case .data(let data):
            try save(data: data, to: url, options: options)
        case .string(let str):
            try save(data: try str.utf8Data(), to: url, options: options)
        case .url(let fileUrl):
            try save(fileUrl: fileUrl, to: url, options: options)
        case .lazyData(let block):
            let data = try block()
            try save(data: data, to: url, options: options)
        }
    }

    func createDirectory(at url: URL) throws {
        try lock.lock {
            try doCreateDirectory(atAbsoluteURL: resolve(url: url))
        }
    }

    @discardableResult func delete(url: URL) throws -> Bool {
        let absolutePath = resolve(url: url).path

        do {
            guard let urlToDelete = try moveItemToTrash(absolutePath: absolutePath) else {
                return false
            }
            try FileManager.default.removeItem(at: urlToDelete)
            return true
        } catch let error {
            throw CompilerError("Failed to delete item at \(absolutePath): \(error.legibleLocalizedDescription)")
        }
    }

    func save(data: Data, to url: URL, options: SaveOptions = .defaultOptions) throws {
        try doSave(to: url, options: options, data: data)
    }

    func save(fileUrl: URL, to url: URL, options: SaveOptions = .defaultOptions) throws {
        // This is less efficient than using FileManager.default.copyItem, but
        // the copyItem method somehow triggers a change notification from watchman
        // on the file that we want to copy. Reading the input file manually and copying
        // it fixes it.
        let data = try Data(contentsOf: fileUrl)
        try doSave(to: url, options: options, data: data)
    }

    func makeTrashItemURL() -> URL {
        return trashDirectoryURL.appendingPathComponent(UUID().uuidString)
    }

    private func moveItemToTrash(absolutePath: String) throws -> URL? {
        return try lock.lock {
            if !FileManager.default.fileExists(atPath: absolutePath) {
                return nil
            }

            let temporaryFileURL = makeTrashItemURL()

            do {
                try FileManager.default.moveItem(atPath: absolutePath, toPath: temporaryFileURL.path)
            } catch let error {
                throw CompilerError("Failed to move item to trash directory: \(error.legibleLocalizedDescription)")
            }

            return temporaryFileURL
        }
    }

    private func doCreateDirectory(atAbsoluteURL absoluteURL: URL) throws {
        let directoryPath = absoluteURL.path

        if !FileManager.default.fileExists(atPath: directoryPath) {
            do {
                if bazel {
                    directoryCheckCache.insert(directoryPath)
                }
                try FileManager.default.createDirectory(atPath: directoryPath, withIntermediateDirectories: true, attributes: nil)
            } catch let error {
                throw CompilerError("Failed to create directory at \(directoryPath): \(error.legibleLocalizedDescription)")
            }
        } else if bazel && !directoryCheckCache.contains(directoryPath) {
            directoryCheckCache.insert(directoryPath)

            if !FileManager.default.isWritableFile(atPath: directoryPath) {
                let attributes = try FileManager.default.attributesOfItem(atPath: directoryPath)

                let mode = (attributes[.posixPermissions] as? NSNumber)?.uint16Value ?? 0
                let newMode = mode | 0o200
                try FileManager.default.setAttributes([.posixPermissions: NSNumber(value: newMode)], ofItemAtPath: directoryPath)
            }
        }
    }

    private func resolve(url: URL) -> URL {
        return url.absoluteURL.standardized
    }

    private func doSave(to url: URL, options: SaveOptions, data: Data) throws {
        guard url.isFileURL else {
            throw CompilerError("Only fileURL's are supported within the ValdiFileManager")
        }
        let absoluteURL = resolve(url: url)
        let absolutePath = absoluteURL.path

        do {
            let directoryURL = absoluteURL.deletingLastPathComponent()
            try createDirectory(at: directoryURL)
            
            if options.contains(.overwriteOnlyIfDifferent) && FileManager.default.fileExists(atPath: absolutePath) {
                
                do {
                    let existingData = try Data(contentsOf: absoluteURL)
                    if existingData == data {
                        return
                    }
                } catch {
                    throw CompilerError("Failed to read existing file: \(error.legibleLocalizedDescription)")
                }

                do {
                    try FileManager.default.removeItem(atPath: absolutePath)
                } catch {
                    throw CompilerError("Failed to remove existing file: \(error.legibleLocalizedDescription)")
                }
            }
            
            try data.write(to: absoluteURL, options: [Data.WritingOptions.atomic])
        } catch {
            throw CompilerError("Failed to save file at '\(absoluteURL.path)': \(error.legibleLocalizedDescription)")
        }
    }

}
