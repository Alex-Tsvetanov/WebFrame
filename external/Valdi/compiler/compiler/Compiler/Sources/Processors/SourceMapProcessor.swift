//
//  SourceMapProcessor.swift
//
//
//  Created by Simon Corsin on 12/15/20.
//

import Foundation
import Zip

private struct CompilatiomItemWithSourceMap {
    let item: CompilationItem
    let sourceMap: String?
}

class SourceMapProcessor: CompilationProcessor {

    static let sourceMapPrefix = "//# sourceMappingURL="

    var description: String {
        return "Generating Source Maps"
    }

    private let outputTarget: OutputTarget
    private let disableMinifyWeb: Bool

    private struct ModuleAndPlatform: Hashable {
        let moduleInfo: CompilationItem.BundleInfo
        let platform: Platform?
    }

    init(compilerConfig: CompilerConfig, disableMinifyWeb: Bool) {
        self.outputTarget = compilerConfig.outputTarget
        self.disableMinifyWeb = disableMinifyWeb
    }

    private func stringFromSourceMappingURL(_ sourceMappingURL: Substring) -> String? {
        guard sourceMappingURL.hasPrefix("data:application/json") else {
            return nil
        }

        guard let base64StartIndex = sourceMappingURL.range(of: "base64,") else {
            return nil
        }

        guard let base64Data = Data(base64Encoded: String(sourceMappingURL[base64StartIndex.upperBound...])) else {
            return nil
        }

        return String(data: base64Data, encoding: .utf8)
    }

    /**
     * Generates a SHA-256 digest based on the items concatenated together.
     * The path of each item is concatenated with the file data.
     * The items are sorted alphabetically (A to Z) by path before being concatenated.
     *
     * Parameters:
     * - items: The items (file + path) to be zipped.
     *
     * Returns:
     * - A String representing the SHA-256 of the provided items.
     */
    private func createHash(items: [CompilationItem]) throws -> String {
        // Only include JavaScript files in the hash
        let javaScriptFileItems = items.compactMap { item -> JavaScriptFile? in
            guard case .javaScript(let jsFile) = item.kind else { return nil }
            return jsFile
        }

        // Sort items alphabethically
        let sortedItems = javaScriptFileItems.sorted { $0.relativePath < $1.relativePath }

        // Join data together and generate SHA-256 digest
        let data = try sortedItems.map { jsFile -> Data in
            let pathData = jsFile.relativePath.data(using: .utf8) ?? Data()
            let fileData = try jsFile.file.readData()
            return pathData + fileData
        }.reduce(Data()) {$0 + $1}
        return try data.generateSHA256Digest()
    }

    private func extractSourceMap(item: SelectedItem<JavaScriptFile>) -> CompilatiomItemWithSourceMap {
        let outItem: CompilationItem
        var sourceMap: String?

        do {
            let jsFileContent = try item.data.file.readString()

            guard let index = jsFileContent.range(of: SourceMapProcessor.sourceMapPrefix) else {
                // Allow an empty sourcemap for web for now
                if disableMinifyWeb && item.item.platform == Platform.web {
                    let jsFileContentWithoutSourceMap = jsFileContent
                    outItem = item.item.with(newKind: .javaScript(JavaScriptFile(file: .data(try jsFileContentWithoutSourceMap.utf8Data()), relativePath: item.data.relativePath)))
                    return CompilatiomItemWithSourceMap(item: outItem, sourceMap: nil)
                }

                throw CompilerError("Could not resolve source map in '\(item.data.relativePath)'")
            }

            let jsFileContentWithoutSourceMap = jsFileContent[jsFileContent.startIndex..<index.lowerBound]

            sourceMap = stringFromSourceMappingURL(jsFileContent[index.upperBound..<jsFileContent.endIndex].trimmed)

            outItem = item.item.with(newKind: .javaScript(JavaScriptFile(file: .data(try jsFileContentWithoutSourceMap.utf8Data()), relativePath: item.data.relativePath)))

        } catch let error {
            outItem = item.item.with(error: error)
        }

        return CompilatiomItemWithSourceMap(item: outItem, sourceMap: sourceMap)
    }

    /**
     * Writes the provided data to the specified URL.
     * Will create the directory if it does not exist.
     * Parameters:
     * - url: The URL to write the data to
     * - data: The data to write
     */
    private func writeFile(to url: URL, data: Data) throws {
        let directoryURL = url.deletingLastPathComponent()
        try FileManager.default.createDirectory(at: directoryURL, withIntermediateDirectories: true, attributes: nil)
        try data.write(to: url)
    }

    /**
     * Sets the last modified date of the file to the provided date.
     * Parameters:
     * - url: The URL of the file
     * - date: The date to set
     */
    private func setLastModifiedDate(for url: URL, date: Date) throws {
        let attributes = [FileAttributeKey.modificationDate: date]
        try FileManager.default.setAttributes(attributes, ofItemAtPath: url.path)
    }

    /**
     * Creates a CompilationItem that will be added to the specified bundle
     * Parameters:
     * - file: The data of the file
     * - path: The path of the file within the bundle
     * - module: The bundle to add the file to
     * - platform: The platform of the file
     * - outputTarget: The output target of the file (debug or release)
     */
    private func createBundleItem(file: File, path: String, module: CompilationItem.BundleInfo, platform: Platform?, outputTarget: OutputTarget) -> CompilationItem {
        let kind : CompilationItem.Kind = .resourceDocument(DocumentResource(outputFilename: path, file: file))
        let sourceURL = module.baseDir.appendingPathComponent(path)
        return CompilationItem(sourceURL: sourceURL, relativeProjectPath: path, kind: kind, bundleInfo: module, platform: platform, outputTarget: outputTarget)
    }

    /**
     * Creates a CompilationItem that will be written out to disk
     * Parameters:
     * - file: The data of the file
     * - path: The URL to write the file to
     * - module: The bundle to add the file to
     * - platform: The platform of the file
     * - outputTarget: The output target of the file (debug or release)
     */
    private func createDiskItem(file: File, path: URL, module: CompilationItem.BundleInfo, platform: Platform?, outputTarget: OutputTarget) -> CompilationItem {
        let kind : CompilationItem.Kind = .finalFile(FinalFile(outputURL: path, file: file, platform: nil, kind: .compiledSource))
        let sourceURL = module.baseDir.appendingPathComponent(path.lastPathComponent)
        return CompilationItem(sourceURL:sourceURL, relativeProjectPath: nil, kind: kind, bundleInfo: module, platform: platform, outputTarget: outputTarget)
    }

    /**
     * Zips the provided sources and source maps
     *
     * Parameters:
     * - sources: The sources to be zipped.
     * - sourceMaps: The source maps to be zipped.
     *
     * Returns:
     * - The zipped data.
     */
    private func zipSourceAndMaps(sources: [CompilationItem], sourceMaps: [String: String]) throws -> Data {
        // Create a temporary directory to store the sources and source maps at the correct patahs
        let dir = URL(fileURLWithPath: NSTemporaryDirectory()).appendingPathComponent(UUID().uuidString)
        let zipFile = dir.appendingPathComponent("maps.zip")

        for source in sources {
            guard case .javaScript(let jsFile) = source.kind else { continue }
            guard let sourceMap = sourceMaps[jsFile.relativePath] else { continue }
            guard let mapFile = try? File.data(sourceMap.utf8Data()) else { continue }

            let sourceURL = dir.appendingPathComponent(jsFile.relativePath)
            let mapURL = sourceURL.appendingPathExtension(FileExtensions.map)

            try writeFile(to: sourceURL, data: try jsFile.file.readData())
            try writeFile(to: mapURL, data: try mapFile.readData())

            // set the last modified date to the fixed date to produce deterministic .zip files
            try setLastModifiedDate(for: sourceURL, date: DeterministicDate.fixedDate)
            try setLastModifiedDate(for: mapURL, date: DeterministicDate.fixedDate)
        }
        // Add all contents of dir to the zip file
        let filesToZip = try FileManager.default.contentsOfDirectory(at: dir, includingPropertiesForKeys: nil, options: [])
        _ = try Zip.zipFiles(paths: filesToZip, zipFilePath: zipFile, password: nil, progress: nil)
        let zipData = try Data(contentsOf: zipFile)

        try FileManager.default.removeItem(at: zipFile)
        try FileManager.default.removeItem(at: dir)

        return zipData
    }

    private func generateSourceMap(moduleAndPlatform: ModuleAndPlatform, outputTarget: OutputTarget, items: [SelectedItem<JavaScriptFile>]) -> [CompilationItem] {
        var outItems: [CompilationItem] = []

        let kind: CompilationItem.Kind

        let module = moduleAndPlatform.moduleInfo
        let platform = moduleAndPlatform.platform

        let outputFilename = "\(module.name).\(FileExtensions.sourceMapJson)"

        do {
            var sourceMaps: [String: String] = [:]

            for item in items where item.item.outputTarget.contains(outputTarget) {
                let itemAndSourceMap = extractSourceMap(item: item)

                if let sourceMap = itemAndSourceMap.sourceMap {
                    sourceMaps[item.data.relativePath] = sourceMap
                }

                outItems.append(itemAndSourceMap.item)
            }

            let jsonData: Data
            if #available(OSX 10.13, *) {
                jsonData = try JSONSerialization.data(withJSONObject: sourceMaps, options: .sortedKeys)
            } else {
                jsonData = try JSONSerialization.data(withJSONObject: sourceMaps, options: [])
            }

            if let saveDir = getSaveDir(platform: platform, module: module, outputTarget: outputTarget)?.appendingPathComponent("source-maps") {
                try FileManager.default.createDirectory(at: saveDir, withIntermediateDirectories: true, attributes: nil)
                let itemsWithoutTestFiles = outItems.filter { !$0.isTestItem }
                if !itemsWithoutTestFiles.isEmpty {
                    let hash = try createHash(items: itemsWithoutTestFiles)
                    let zippedMaps = try zipSourceAndMaps(sources: itemsWithoutTestFiles, sourceMaps: sourceMaps)
                    let zippedMapsURL = saveDir.appendingPathComponent("\(hash).\(FileExtensions.zip)")
                    outItems.append(createDiskItem(file: .data(zippedMaps), path: zippedMapsURL, module: module, platform: platform, outputTarget: outputTarget))
                    outItems.append(createBundleItem(file: .string(hash), path: Files.hash, module: module, platform: platform, outputTarget: outputTarget))
                }
            }

            kind = .rawResource(.data(jsonData))
        } catch let error {
            kind = .error(error, originalItemKind: .rawResource(.data(Data())))
        }

        outItems.append(CompilationItem(sourceURL: module.baseDir.appendingPathComponent(outputFilename), relativeProjectPath: nil, kind: kind, bundleInfo: module, platform: platform, outputTarget: outputTarget))

        return outItems
    }

    private func getSaveDir(platform: Platform?, module: CompilationItem.BundleInfo, outputTarget: OutputTarget) -> URL? {
        switch platform {
        case .ios:
            if outputTarget.contains(.debug) {
                return module.iosDebugOutputDirectories?.baseURL
            } else if outputTarget.contains(.release) {
                return module.iosReleaseOutputDirectories?.baseURL
            }
        case .android:
            if outputTarget.contains(.debug) {
                return module.androidDebugOutputDirectories?.baseURL
            } else if outputTarget.contains(.release) {
                return module.androidReleaseOutputDirectories?.baseURL
            }
        default:
            return nil
        }
        return nil
    }

    private func generateSourceMaps(items: GroupedItems<ModuleAndPlatform, SelectedItem<JavaScriptFile>>) -> [CompilationItem] {
        var out = [CompilationItem]()

        if outputTarget.contains(.debug) {
            out += generateSourceMap(moduleAndPlatform: items.key, outputTarget: .debug, items: items.items)
        }
        if outputTarget.contains(.release) {
            out += generateSourceMap(moduleAndPlatform: items.key, outputTarget: .release, items: items.items)
        }

        return out
    }

    func process(items: CompilationItems) throws -> CompilationItems {
        return items.select { (item) -> JavaScriptFile? in
            switch item.kind {
            case .javaScript(let jsFile):
                return jsFile
            default:
                return nil
            }
        }.groupBy { (item) -> ModuleAndPlatform in
            return ModuleAndPlatform(moduleInfo: item.item.bundleInfo, platform: item.item.platform)
        }.transformEach(generateSourceMaps)
    }

}
