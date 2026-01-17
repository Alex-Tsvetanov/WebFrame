//
//  ValdiModuleUtils.swift
//  Compiler
//
//  Created by Simon Corsin on 8/12/19.
//

import Foundation
import SwiftProtobuf

class ValdiModuleUtils {

    static func buildModule(logger: ILogger, fileManager: ValdiFileManager, paths: [String], baseUrl: URL, out: String?) throws {
        let outputURL = try CLIUtils.getOutputURLs(commandName: "--build-module", baseUrl: baseUrl, out: out)

        let zippableItems: [ZippableItem] = paths.map {
            ZippableItem(file: .url(baseUrl.resolving(path: $0)), path: $0)
        }

        logger.info("Saving Valdi module...")

        let moduleData = try ValdiModuleBuilder(items: zippableItems).build()

        logger.info("Writing Valdi module to \(outputURL.path)")
        try fileManager.save(data: moduleData, to: outputURL)
    }

    static func unpackModule(logger: ILogger, fileManager: ValdiFileManager, paths: [String], baseUrl: URL, out: String?) throws {
        let outputURL = try CLIUtils.getOutputURLs(commandName: "--unpack-module", baseUrl: baseUrl, out: out)

        logger.info("Unpacking Valdi module...")

        for path in paths {
            let url = baseUrl.resolving(path: path)
            try unpackModule(logger: logger, fileManager: fileManager, url: url, outputURL: outputURL)
        }
    }

    static func textconvModule(logger: ILogger, fileManager: ValdiFileManager, paths: [String], baseUrl: URL, out: String?) throws {
        let outputURL = try CLIUtils.getOutputURLs(commandName: "--textconv-module", baseUrl: baseUrl, out: out)

        logger.info("Textconverting Valdi module...")

        for path in paths {
            let url = baseUrl.resolving(path: path)
            try unpackModule(logger: logger, fileManager: fileManager, url: url, outputURL: outputURL, transform: textconvItem)
        }
    }

    private static func unpackModule(logger: ILogger, fileManager: ValdiFileManager, url: URL, outputURL: URL, transform: ((ZippableItem) throws -> ZippableItem) = { $0 }) throws {
        let moduleData = try Data(contentsOf: url)
        let items = try ValdiModuleBuilder.unpack(module: moduleData)
        let transformedItems = try items.map(transform)
        for item in transformedItems {
            let outputFileURL = outputURL.appendingPathComponent(item.path)
            logger.info("Saving \(outputFileURL.path)")
            try fileManager.save(file: item.file, to: outputFileURL)
        }
    }

    private static func textconvItem(item: ZippableItem) throws -> ZippableItem {
        let filename = item.path
        let fileData = try item.file.readData()

        let protobufMessage: Message?
        if filename.hasSuffix(Files.downloadManifest) {
            protobufMessage = try Valdi_DownloadableModuleManifest(serializedData: fileData)
        } else if filename.hasSuffix(FileExtensions.valdiCss) {
            protobufMessage = try Valdi_StyleNode(serializedData: fileData)
        } else {
            protobufMessage = nil
        }

        let dataToUse: Data
        if let foundMessage = protobufMessage {
            var options = TextFormatEncodingOptions()
            options.printUnknownFields = true
            let stringToUse = foundMessage.textFormatString(options: options)
            dataToUse = stringToUse.data(using: .utf8) ?? Data()
        } else if filename.hasSuffix(FileExtensions.assetCatalog) {
            let records = try parseBinaryAssetCatalog(fileData: fileData)
            let lines = records.map { "- \($0.fileName): \($0.width)x\($0.height)" }
            dataToUse = lines.joined(separator: "\n").data(using: .utf8) ?? Data()
        } else {
            dataToUse = fileData
        }

        return ZippableItem(file: .data(dataToUse), path: filename)
    }

    private struct BinaryAssetCatalogRecord {
        let width: UInt32
        let height: UInt32
        let fileName: String
    }

    private static func parseBinaryAssetCatalog(fileData: Data) throws -> [BinaryAssetCatalogRecord] {
        let assetCatalogParser = Parser(sequence: fileData)
        guard try assetCatalogParser.parse(subsequence: Magic.valdiMagic) else {
            throw CompilerError("Did not find valdi magic in asset catalog data")
        }

        let assetCatalogDataLength = try assetCatalogParser.parseInt()
        if fileData.count != assetCatalogDataLength + 8 {
            throw CompilerError("Unexpected asset catalog data length \(fileData.count), expected \(assetCatalogDataLength)")
        }
        var records: [BinaryAssetCatalogRecord] = []
        while !assetCatalogParser.isAtEnd {
            let assetFileName = String(data: try assetCatalogParser.parseValdiData(), encoding: .utf8) ?? "<invalid>"

            let widthHeightData = try assetCatalogParser.parseValdiData()

            let widthData = widthHeightData[widthHeightData.startIndex..<widthHeightData.startIndex+4]
            let heightData = widthHeightData[widthHeightData.startIndex+4..<widthHeightData.startIndex+8]
            let width = UInt32(data: widthData) ?? 0
            let height = UInt32(data: heightData) ?? 0
            let record = BinaryAssetCatalogRecord(width: width,
                                                  height: height,
                                                  fileName: assetFileName)
            records.append(record)
        }
        return records
    }

}
