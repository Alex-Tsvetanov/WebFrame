//
//  FinalFile.swift
//  Compiler
//
//  Created by Simon Corsin on 7/24/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

import Foundation

struct FinalFile {

    let outputURL: URL
    let file: File
    let platform: Platform?
    let kind: Kind

    enum Kind {
        case image(scale: Double?, isRemote: Bool)
        case compiledSource
        case nativeSource
        case unknown
        case assetsPackage
        case dependencyInjectionData
    }

    var shouldBundle: Bool {
        return platform != nil
    }

    func outputPath(relativeBundleURL: URL) -> String {
        var path = relativeBundleURL
            .deletingLastPathComponent()
            .appendingPathComponent(outputURL.lastPathComponent)
            .standardized.path

        // On Linux URL.standardized doesn't remove "./"
        while path.hasPrefix("./") {
            path = String(path[path.index(path.startIndex, offsetBy: 2)...])
        }

        return path
    }
}
