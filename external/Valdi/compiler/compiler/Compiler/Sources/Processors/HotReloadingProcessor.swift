//
//  HotReloadingProcessor.swift
//  Compiler
//
//  Created by Simon Corsin on 7/24/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

import Foundation

// [.finalFile] -> [.finalFile]
class HotReloadingProcessor: CompilationProcessor {

    var description: String {
        return "Notifying Document changes over the network"
    }

    private struct FinalImage {
        let scale: Double
        let data: Data

    }

    private let logger: ILogger
    private let daemonService: DaemonService

    init(logger: ILogger, daemonService: DaemonService) {
        self.logger = logger
        self.daemonService = daemonService
    }

    func process(items: CompilationItems) throws -> CompilationItems {
        var allAddedResource = [Resource]()

        for item in items.allItems {
            guard case .finalFile(let finalFile) = item.kind, item.outputTarget.contains(.debug) else {
                continue
            }

            switch finalFile.kind {
            case .compiledSource, .assetsPackage:
                do {
                    let data = try finalFile.file.readData()
                    let resource = Resource(item: item, finalFile: finalFile, data: data)
                    allAddedResource.append(resource)
                } catch let error {
                    logger.error("Failed to hot reload file \(item.bundleInfo.name)/\(item.relativeBundleURL.path) : \(error.legibleLocalizedDescription)")
                }
            case .nativeSource, .unknown, .image, .dependencyInjectionData:
                // Only compiled sources and assets packages can be handled
                break
            }
        }

        if !allAddedResource.isEmpty {
            daemonService.resourcesDidChange(resources: allAddedResource)
        }

        return items
    }
}
