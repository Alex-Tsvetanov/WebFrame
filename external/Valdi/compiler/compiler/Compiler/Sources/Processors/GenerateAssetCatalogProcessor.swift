//
//  GenerateAssetCatalogProcessor.swift
//  
//
//  Created by Simon Corsin on 2/13/20.
//

import Foundation

struct AssetCatalogEntry {
    let projectRelativeDirectoryPath: String
    let virtualPath: String
    let image: ImageAsset?
    let font: FontAsset?
}

struct AssetCatalogTypes {
    static let image: UInt32 = 1
    static let font: UInt32 = 2
}

class GenerateAssetCatalogProcessor: CompilationProcessor {

    var description: String {
        return "Generating Asset Catalogs"
    }

    private let logger: ILogger
    private let fileManager: ValdiFileManager
    private let projectConfig: ValdiProjectConfig
    private let targetSizeForPreview = 30
    private let previousAssets = Synchronized<[String: [AssetCatalogEntry]]>(data: [:])
    private let enablePreviewInGeneratedTSFile: Bool

    init(logger: ILogger, fileManager: ValdiFileManager, projectConfig: ValdiProjectConfig, enablePreviewInGeneratedTSFile: Bool) {
        self.logger = logger
        self.fileManager = fileManager
        self.projectConfig = projectConfig
        self.enablePreviewInGeneratedTSFile = enablePreviewInGeneratedTSFile
    }

    private func findOptimalVariantForPreview(variants: [ImageAssetVariant]) -> ImageAssetVariant? {
        var best: ImageAssetVariant?
        var bestTargetOffset = Int.max

        for variant in variants {
            let variantSize = max(variant.imageInfo.size.width, variant.imageInfo.size.height)
            let targetOffset = abs(targetSizeForPreview - variantSize)

            if best == nil || targetOffset < bestTargetOffset {
                best = variant
                bestTargetOffset = targetOffset
            }
        }

        return best
    }

    private func generateTypeScriptModule(relativeAssetDirectoryPath: String, assets: [AssetCatalogEntry]) -> String {
        let body = CodeWriter()

        for (index, asset) in assets.enumerated() {
            if index > 0 {
                body.appendBody("\n\n")
            }

            if let image = asset.image {
                let propertyName = image.identifier.assetName.camelCased

                if enablePreviewInGeneratedTSFile {
                    let bestVariant = findOptimalVariantForPreview(variants: image.variants)

                    let absolutePath: String
                    switch bestVariant?.file {
                    case .url(let url):
                        absolutePath = url.path
                    default:
                        absolutePath = "<null>"
                    }

                    // Adding additinal empty lines in the comment to workaround an issue with VSCode preview
                    // where it doesn't make the preview window big enough to show the image.

                    body.appendBody("""
                    /**
                    * ![Preview](\(absolutePath))
                    *
                    *
                    *
                    *
                    * .
                    */

                    """)
                }

                body.appendBody("readonly \(propertyName): Asset;")
            } else if let font = asset.font {
                if enablePreviewInGeneratedTSFile {
                    body.appendBody("""
                    /**
                    * Font reference for \(asset.virtualPath)
                    */
                    """)
                }
                let propertyName = font.name.camelCased
                body.appendBody("\n\(propertyName)(size: number): string;")
            }
        }

        let tsOutput = CodeWriter()
        tsOutput.appendBody("""
            import { loadCatalog } from 'valdi_core/src/AssetCatalog';
            import { Asset } from 'valdi_core/src/Asset';

            /**
            * Assets Catalog Module generated from files in \(relativeAssetDirectoryPath)
            */
            interface AssetCatalog {
                \(body.content)
            }

            const catalog = loadCatalog('\(relativeAssetDirectoryPath)') as AssetCatalog;
            export default catalog;
            """)

        return tsOutput.content.indented(indentPattern: "  ")
    }

    private func generateCatalogBin(assets: [AssetCatalogEntry]) throws -> Data {
        var items = [ZippableItem]()

        for asset in assets {
            var data = Data()

            let path: String
            if let image = asset.image {
                data.append(integer: UInt32(image.size.width))
                data.append(integer: UInt32(image.size.height))
                data.append(integer: AssetCatalogTypes.image)

                path = image.identifier.assetName
            } else if let font = asset.font {
                data.append(integer: UInt32(0))
                data.append(integer: UInt32(0))
                data.append(integer: AssetCatalogTypes.font)

                path = font.name
            } else {
                fatalError()
            }

            let item = ZippableItem(file: .data(data), path: path)
            items.append(item)
        }

        let module = ValdiModuleBuilder(items: items)
        module.compress = false

        return try module.build()
    }

    private func resolveAssets(key: String, inputAssets: [AssetCatalogEntry]) -> [AssetCatalogEntry] {
        var allAssets: [AssetCatalogEntry]

        if let previousAssets = self.previousAssets.data({ (previousAssets) -> [AssetCatalogEntry]? in
            return previousAssets[key]
        }) {
            allAssets = inputAssets

            var seenIdentifiers: Set<String> = []
            inputAssets.forEach { seenIdentifiers.insert($0.virtualPath) }

            allAssets += previousAssets.filter { !seenIdentifiers.contains($0.virtualPath) }
        } else {
            allAssets = inputAssets
        }

        self.previousAssets.data { (previousAssets) -> Void in
            previousAssets[key] = allAssets
        }

        return allAssets.sorted(by: { (left, right) in left.virtualPath < right.virtualPath })
    }

    private func generateAssetCatalog(assetsInBundle: GroupedItems<String, SelectedItem<AssetCatalogEntry>>) -> [CompilationItem] {
        let assetDirectoryPath = assetsInBundle.key
        let assets = resolveAssets(key: assetDirectoryPath, inputAssets: assetsInBundle.items.map { $0.data })

        var outItems = assetsInBundle.items.map { $0.item }
        let anyItem = outItems[0]

        do {
            let tsModule = generateTypeScriptModule(relativeAssetDirectoryPath: assetDirectoryPath, assets: assets)
            let tsModuleData = try tsModule.utf8Data()

            let catalogData = try generateCatalogBin(assets: assets)

            let catalogURL = anyItem.bundleInfo.absoluteURL(forRelativeProjectPath: "\(assetDirectoryPath).\(FileExtensions.assetCatalog)")

            let assetFilename = "\(assetDirectoryPath).\(FileExtensions.typescript)"

            // Save the file into the generated_ts directory to get autocompletion in VSCode
            let outputTsURL = projectConfig.generatedTsDirectoryURL.appendingPathComponent(assetFilename)
            try fileManager.save(data: tsModuleData, to: outputTsURL)

            // Also generate an item so we can actually compile it
            let emittedTsSourceURL = anyItem.bundleInfo.absoluteURL(forRelativeProjectPath: assetFilename)
            let emittedTsItem = CompilationItem(sourceURL: emittedTsSourceURL, relativeProjectPath: assetFilename, kind: .typeScript(.data(tsModuleData), emittedTsSourceURL), bundleInfo: anyItem.bundleInfo, platform: nil, outputTarget: .all)
            let emittedCatalogItemIOS = CompilationItem(sourceURL: catalogURL, relativeProjectPath: nil, kind: .finalFile(FinalFile(outputURL: catalogURL, file: .data(catalogData), platform: .ios, kind: .compiledSource)), bundleInfo: anyItem.bundleInfo, platform: .ios, outputTarget: .all)
            let emittedCatalogItemAndroid = CompilationItem(sourceURL: catalogURL, relativeProjectPath: nil, kind: .finalFile(FinalFile(outputURL: catalogURL, file: .data(catalogData), platform: .android, kind: .compiledSource)), bundleInfo: anyItem.bundleInfo, platform: .android, outputTarget: .all)

            outItems.append(emittedTsItem)
            outItems.append(emittedCatalogItemIOS)
            outItems.append(emittedCatalogItemAndroid)

            logger.debug("-- Generated Asset Catalog from assets in \(assetDirectoryPath)")
        } catch let error {
            let errorMessage = "Failed to generate Asset Catalog: \(error.legibleLocalizedDescription)"
            logger.error(errorMessage)
            let newError = CompilerError(errorMessage)

            return outItems.map { $0.with(error: newError) }
        }

        return outItems
    }

    func process(items: CompilationItems) throws -> CompilationItems {
        return items.select { (item) -> AssetCatalogEntry? in
            if case .imageAsset(let asset) = item.kind {
                return AssetCatalogEntry(projectRelativeDirectoryPath: asset.identifier.relativeProjectAssetDirectoryPath, virtualPath: asset.identifier.virtualPath, image: asset, font: nil)
            }
            if case .resourceDocument(let document) = item.kind, document.outputFilename.hasAnyExtension(FileExtensions.fonts) {
                return AssetCatalogEntry(projectRelativeDirectoryPath: item.relativeProjectPath.deletingLastPathComponent(), virtualPath: item.relativeProjectPath, image: nil, font: FontAsset(name: document.outputFilename.deletingPathExtension(), data: document.file))
            }
            return nil
        }.groupBy { (item) -> String in
            return item.data.projectRelativeDirectoryPath
        }.transformEachConcurrently(generateAssetCatalog)
    }

}
