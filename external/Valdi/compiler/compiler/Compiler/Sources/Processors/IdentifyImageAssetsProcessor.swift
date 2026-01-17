//
//  IdentifyImageAssetsProcessor.swift
//  
//
//  Created by Simon Corsin on 2/12/20.
//

import Foundation

private struct IdentifiedImage {
    let resolvedVariant: ResolvedImageVariant
    let imageInfo: ImageInfo
    let url: URL

    var densityIndependentSize: ImageSize {
        return imageInfo.size.scaled(1.0 / resolvedVariant.variant.scale)
    }
}

private struct IdentifiedImageItem {
    let item: SelectedItem<URL>
    let image: IdentifiedImage
}

// [.rawResource] -> [.imageAsset]
class IdentifyImageAssetsProcessor: CompilationProcessor {

    var description: String {
        return "Identifying Image assets"
    }

    private let logger: ILogger
    private let compilerConfig: CompilerConfig

    private let imageToolbox: ImageToolbox
    private let lock = DispatchSemaphore.newLock()
    private var previousIdentifiedItemsByPath: [String: [IdentifiedImageItem]] = [:]
    private static let maxImageSizeDeviation = 2
    private let cache: DiskCache?

    init(logger: ILogger, imageToolbox: ImageToolbox, compilerConfig: CompilerConfig, diskCacheProvider: DiskCacheProvider) throws {
        self.logger = logger
        self.imageToolbox = imageToolbox
        self.compilerConfig = compilerConfig
        if diskCacheProvider.isEnabled() {
            let version = try imageToolbox.getVersion()
            self.cache = diskCacheProvider.newCache(cacheName: "identify_images", outputExtension: "json", metadata: ["image_toolbox_version": version])
        } else {
            self.cache = nil
        }
    }

    private func getImageInfo(item: CompilationItem, inputURL: URL) throws -> ToolboxExecutable.ImageInfoOutput {
        guard let cache else {
            return try imageToolbox.getInfo(inputPath: inputURL.path)
        }

        let inputData = try File.url(inputURL).readData()

        let output = cache.getOutput(compilationItem: item, inputData: inputData)

        if let output, let imageInfoOutput = try? ToolboxExecutable.ImageInfoOutput.fromJSON(output) {
            return imageInfoOutput
        }

        let imageInfoOutput = try imageToolbox.getInfo(inputPath: inputURL.path)

        try cache.setOutput(compilationItem: item, inputData: inputData, outputData: try imageInfoOutput.toJSON())

        return imageInfoOutput
    }

    private func identifyImage(item: CompilationItem, imageURL: URL) -> Promise<IdentifiedImage> {
        do {
            let variant = try ImageVariantResolver.resolveVariant(imageURL: imageURL, relativeProjectPath: item.relativeProjectPath)

            let assetName = variant.assetIdentifier.assetName

            if assetName.lowercased() != assetName {
                throw CompilerError("Invalid filename '\(assetName)', image filenames need to be lowercased")
            }

            let imageInfo: ImageInfo
            if compilerConfig.generateTSResFiles {
                // Image size is not needed when just generating TS res files
                imageInfo = ImageInfo(size: ImageSize(width: 0, height: 0))
            } else {
                let info = try getImageInfo(item: item, inputURL: imageURL)
                imageInfo = ImageInfo(size: ImageSize(width: info.width, height: info.height))
            }

            return Promise(data: IdentifiedImage(resolvedVariant: variant, imageInfo: imageInfo, url: imageURL))
        } catch let error {
            return Promise(error: error)
        }
    }

    private func getIdentifiedItemsByPath(items: [SelectedItem<URL>], resolvedImagePromises: [Promise<IdentifiedImage>], out: inout [CompilationItem]) -> [String: [IdentifiedImageItem]] {
        var identifiedItemsByPath: [String: [IdentifiedImageItem]] = [:]
        let previousIdentifiedItemsByPath = lock.lock { self.previousIdentifiedItemsByPath }

        for (index, item) in items.enumerated() {
            let imagePromise = resolvedImagePromises[index]

            do {
                let resolvedImage = try imagePromise.waitForData()

                let virtualPath = resolvedImage.resolvedVariant.assetIdentifier.virtualPath

                var identifiedItems = identifiedItemsByPath[virtualPath, default: []]

                if identifiedItems.isEmpty {
                    // Re-add previously identified items, so that we can support hot reload on images
                    if let previousItems = previousIdentifiedItemsByPath[virtualPath] {
                        // Only re-add the items that have a different url than the one we're processing
                        for previousItem in previousItems where previousItem.item.data != item.data {
                            identifiedItems.append(previousItem)
                        }
                    }
                }

                identifiedItems.append(IdentifiedImageItem(item: item, image: resolvedImage))

                identifiedItemsByPath[virtualPath] = identifiedItems
            } catch let error {
                out.append(item.item.with(error: error))
            }
        }

        return identifiedItemsByPath
    }

    private func validateImageVariantsSizes(items: [IdentifiedImageItem], densityIndependentSize: ImageSize) throws {
        var failed = false

        for item in items {
            let currentDPSize = item.image.densityIndependentSize

            let widthDeviation = abs(densityIndependentSize.width - currentDPSize.width)
            let heightDeviation = abs(densityIndependentSize.height - currentDPSize.height)
            if widthDeviation > IdentifyImageAssetsProcessor.maxImageSizeDeviation || heightDeviation > IdentifyImageAssetsProcessor.maxImageSizeDeviation {
                failed = true
                break
            }
        }

        if failed {
            let detailMessage: String = items.map {
                let dp = $0.image.densityIndependentSize
                let pixels = $0.image.imageInfo.size
                return "\($0.item.item.sourceURL.path): \(dp.width)x\(dp.height)dp (\(pixels.width)x\(pixels.height)px)"
            }.joined(separator: "\n")
            let errorMessage = "Incorrect image sizes, the density independent size of those images varies by more than \(IdentifyImageAssetsProcessor.maxImageSizeDeviation)dp:\n\(detailMessage)\n\nPlease find and remove the incorrect images."
            throw CompilerError(errorMessage)
        }
    }

    private func createImageAssetItems(identifiedItemsByPath: [String: [IdentifiedImageItem]]) -> [CompilationItem] {
        var out = [CompilationItem]()

        for (virtualPath, items) in identifiedItemsByPath {
            lock.lock {
                 self.previousIdentifiedItemsByPath[virtualPath] = items
             }

            // Take the highest available scale our base
            let sortedItems = items.sorted(by: { (left, right) in
                if left.image.resolvedVariant.variant.scale == right.image.resolvedVariant.variant.scale {
                    // Use the file path on disk to sort in case of equality (e.g. iOS @3x with Android xxhdpi)
                    return left.item.data.absoluteString > right.item.data.absoluteString
                } else {
                    return left.image.resolvedVariant.variant.scale > right.image.resolvedVariant.variant.scale
                }
            })
            let highestItem = sortedItems.last!
            let imageSize = highestItem.image.densityIndependentSize

            do {
                if !compilerConfig.generateTSResFiles {
                    try validateImageVariantsSizes(items: sortedItems, densityIndependentSize: imageSize)
                }

                var allVariants = items.map { ImageAssetVariant(imageInfo: $0.image.imageInfo, file: .url($0.image.url), variantSpecs: $0.image.resolvedVariant.variant) }
                allVariants.sort { left, right in
                    return left.variantSpecs.scale < right.variantSpecs.scale
                }

                let imageAsset = ImageAsset(identifier: highestItem.image.resolvedVariant.assetIdentifier, size: imageSize, variants: allVariants)
                out.append(highestItem.item.item.with(newKind: .imageAsset(imageAsset)))
            } catch let error {
                logger.error(error.legibleLocalizedDescription)
                out.append(highestItem.item.item.with(error: error))
            }
        }

        return out
    }

    private func identifyAssets(items: [SelectedItem<URL>]) -> [CompilationItem] {
        let filteredItems = items.filter { selectedItem in
            if !compilerConfig.onlyProcessResourcesForModules.isEmpty && !compilerConfig.onlyProcessResourcesForModules.contains(selectedItem.item.bundleInfo.name) {
                return false
            }
            return true
        }

        // Step 1: identify all images and find their size
        let resolvedImagePromises = filteredItems.parallelMap { self.identifyImage(item: $0.item, imageURL: $0.data) }

        var out = [CompilationItem]()

        // Step 2: Group the images by their asset identifier

        let identifiedItemsByPath = getIdentifiedItemsByPath(items: filteredItems, resolvedImagePromises: resolvedImagePromises, out: &out)

        // Step 3: Generate Compilation items backed by ImageAsset's

        out += createImageAssetItems(identifiedItemsByPath: identifiedItemsByPath)

        return out
    }

    func process(items: CompilationItems) throws -> CompilationItems {
        return items.select { (item) -> URL? in
            if case .rawResource(let file) = item.kind, case .url(let url) = file, FileExtensions.exportedImages.contains(url.pathExtension) {
                return url
            }
            return nil
        }
        .transformAll(identifyAssets)
    }

}
