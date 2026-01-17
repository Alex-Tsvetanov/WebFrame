//
//  ImageAsset.swift
//  
//
//  Created by Simon Corsin on 2/12/20.
//

import Foundation

struct ImageInfo: Equatable, Codable {
    let size: ImageSize
}

struct ImageSize: Equatable, Codable, CustomStringConvertible {
    let width: Int
    let height: Int

    var cgSize: CGSize {
        return CGSize(width: Double(width), height: Double(height))
    }

    var description: String {
        return "\(width)x\(height)"
    }

    func scaled(_ scale: Double) -> ImageSize {
        let scaledWidth = Int((Double(width) * scale).rounded())
        let scaledheight = Int((Double(height) * scale).rounded())

        return ImageSize(width: scaledWidth, height: scaledheight)
    }
}

struct ImageAssetVariant {
    let imageInfo: ImageInfo
    let file: File
    let variantSpecs: ImageVariantSpecs
}

struct ImageAssetIdentifier {
    /**
     A name identifying an asset accross all variants
     */
    let assetName: String

    /**
     Root directory which this asset lives, relative to the project path
     */
    let relativeProjectAssetDirectoryPath: String

    /**
     A virtual path which uniquely identifies an asset on the system
     across all variants.
     */
    var virtualPath: String {
        return "\(relativeProjectAssetDirectoryPath)/\(assetName)"
    }
}

struct ImageAsset {
    let identifier: ImageAssetIdentifier
    let size: ImageSize
    let variants: [ImageAssetVariant]
}

extension ImageAsset {

    var bestVariant: ImageAssetVariant? {
        return ImageAsset.findHighestVariant(variants: variants)
    }

    static func findHighestVariant(variants: [ImageAssetVariant]) -> ImageAssetVariant? {
        // Find if we have an SVG available, if we do, we use it for generating images
        if let svgVariant = variants.first(where: { $0.variantSpecs.fileExtension == FileExtensions.svg }) {
            return svgVariant
        }

        // Otherwise, find the variant with the highest scale
        return variants.max { (lhs, rhs) -> Bool in
            let left = lhs.variantSpecs
            let right = rhs.variantSpecs
            if left.scale > right.scale {
                return false
            } else if left.platform == .ios && right.platform == .android {
                // If scales are equal, prefer the iOS png to stabilize the sort
                return false
            } else {
                return true
            }
        }
    }

}
