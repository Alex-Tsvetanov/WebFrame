//
//  ResourceStore.swift
//  Compiler
//
//  Created by saniul on 27/12/2018.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

import Foundation

struct Resource {
    let item: CompilationItem
    let finalFile: FinalFile
    let data: Data

    let filePathInBundle: String

    init(item: CompilationItem, finalFile: FinalFile, data: Data) {
        self.item = item
        self.finalFile = finalFile
        self.data = data

        filePathInBundle = Resource.getFilePathInBudle(item: item, finalFile: finalFile)
    }

    static func getFilePathInBudle(item: CompilationItem, finalFile: FinalFile) -> String {
        return finalFile.outputPath(relativeBundleURL: item.relativeBundleURL)
    }
}

/**
 Store all compiled resources by file path
 */
final class ResourceStore {

    private let resourcesLock = DispatchSemaphore.newLock()

    private var resources = [String: Resource]()

    var allResources: [Resource] {
        return resourcesLock.lock { Array(resources.values) }
    }

    func addResource(_ resource: Resource) {
        let platformPrefix = resource.finalFile.platform.map { "\($0)" } ?? "unknown"
        let filePath = resource.item.relativeBundleURL
            .deletingLastPathComponent()
            .appendingPathComponent(resource.finalFile.outputURL.lastPathComponent)
            .standardized.path
        let bundleName = resource.item.bundleInfo.name
        let fullPath = "\(platformPrefix)/\(bundleName)/\(filePath)"

        resourcesLock.lock {
            resources[fullPath] = resource
        }
    }
}
