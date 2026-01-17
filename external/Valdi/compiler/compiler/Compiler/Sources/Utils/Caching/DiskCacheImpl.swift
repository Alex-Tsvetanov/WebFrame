//
//  DiskCache.swift
//  
//
//  Created by Simon Corsin on 10/22/19.
//

import Foundation

struct DiskCacheOutputWithURL {
    let url: URL
    let data: Data
}

class DiskCacheImpl: DiskCache {

    private let logger: ILogger
    private let fileManager: ValdiFileManager
    private let baseDirectoryURL: URL
    private let inputDirectoryURL: URL
    private let outputDirectoryURL: URL
    private let outputExtension: String

    init(logger: ILogger, fileManager: ValdiFileManager, projectConfig: ValdiProjectConfig, cacheName: String, outputExtension: String, metadata: [String: String]) {
        self.logger = logger
        self.fileManager = fileManager
        let cachesRootURL = projectConfig.buildDirectoryURL.appendingPathComponent("caches", isDirectory: true)
        self.baseDirectoryURL = cachesRootURL.appendingPathComponent(cacheName, isDirectory: true)
        self.inputDirectoryURL = baseDirectoryURL.appendingPathComponent("input", isDirectory: true)
        self.outputDirectoryURL = baseDirectoryURL.appendingPathComponent("output", isDirectory: true)
        self.outputExtension = outputExtension

        try? fileManager.createDirectory(at: baseDirectoryURL)

        let cacheMetadataURL = inputDirectoryURL.appendingPathComponent("cache.json", isDirectory: false)

        if let cacheMedata = getCachedMetadata(url: cacheMetadataURL), cacheMedata == metadata {
            // All good
        } else {
            // Cache is invalid

            do {
                try fileManager.delete(url: inputDirectoryURL)
                try fileManager.delete(url: outputDirectoryURL)

                let data = try JSONSerialization.data(withJSONObject: metadata, options: [])
                try fileManager.save(data: data, to: cacheMetadataURL)
            } catch let error {
                logger.error("Failed to write cache metadata: \(error.legibleLocalizedDescription)")
            }
        }
    }

    private func getCachedMetadata(url: URL) -> [String: String]? {
        guard let data = try? Data(contentsOf: url) else {
            return nil
        }

        guard let object = try? JSONSerialization.jsonObject(with: data, options: []) as? [String: String] else {
            return nil
        }

        return object
    }

    private func getCacheURLs(item: String) -> (inputURL: URL, outputURL: URL) {
        var outputURL = outputDirectoryURL.appendingPathComponent(item, isDirectory: false)
        outputURL.deletePathExtension()
        outputURL.appendPathExtension(outputExtension)

        return (
            inputDirectoryURL.appendingPathComponent(item, isDirectory: false),
            outputURL
        )
    }

    private func getItemName(item: String, platform: Platform?, target: OutputTarget) -> String {
        if let platform = platform {
            return "\(target.description)/platform_\(platform)/\(item)"
        } else {
            return "\(target.description)/platform_all/\(item)"
        }
    }

    func getOutput(compilationItem: CompilationItem, inputData: Data) -> Data? {
        return getOutput(item: compilationItem.relativeProjectPath, platform: compilationItem.platform, target: compilationItem.outputTarget, inputData: inputData)
    }

    func getOutput(item: String, platform: Platform?, target: OutputTarget, inputData: Data) -> Data? {
        return getOutput(item: getItemName(item: item, platform: platform, target: target), inputData: inputData)
    }

    func getOutput(item: String, inputData: Data) -> Data? {
        let urls = getCacheURLs(item: item)

        guard let cacheData = try? Data(contentsOf: urls.inputURL), cacheData == inputData else {
            return nil
        }

        guard let outputData = try? Data(contentsOf: urls.outputURL) else {
            return nil
        }

        return outputData
    }

    func getExpectedOutputURL(item: String) -> URL {
        return getCacheURLs(item: item).outputURL
    }

    func getURL() -> URL {
        return baseDirectoryURL
    }

    func setOutput(compilationItem: CompilationItem, inputData: Data, outputData: Data) throws {
        try setOutput(item: compilationItem.relativeProjectPath, platform: compilationItem.platform, target: compilationItem.outputTarget, inputData: inputData, outputData: outputData)
    }

    func setOutput(item: String, platform: Platform?, target: OutputTarget, inputData: Data, outputData: Data) throws {
        try setOutput(item: getItemName(item: item, platform: platform, target: target), inputData: inputData, outputData: outputData)
    }

    func setOutput(item: String, inputData: Data, outputData: Data) throws {
        let urls = getCacheURLs(item: item)

        try fileManager.save(data: inputData, to: urls.inputURL)
        try fileManager.save(data: outputData, to: urls.outputURL)
    }

    func removeOutput(item: String) {
        let urls = getCacheURLs(item: item)

        _ = try? fileManager.delete(url: urls.inputURL)
        _ = try? fileManager.delete(url: urls.outputURL)
    }

}
