//
//  FilterItemsProcessor.swift
//  Compiler
//
//  Created by Simon Corsin on 12/26/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

import Foundation

// [.unknown] -> [.unknown]
class FilterItemsProcessor: CompilationProcessor {

    var description: String {
        return "Filtering items"
    }

    private let modulesFilter: ModulesFilter?
    private let hotreloadingEnabled: Bool

    init(modulesFilter: ModulesFilter?, hotreloadingEnabled: Bool = false) {
        self.modulesFilter = modulesFilter
        self.hotreloadingEnabled = hotreloadingEnabled
    }

    private func resolveInclusion(item: CompilationItem) -> Bool {
        if let modulesFilter = modulesFilter {
            if !modulesFilter.shouldIncludeInInput(bundleInfo: item.bundleInfo) {
                return false
            }
        }

        if (self.hotreloadingEnabled) {
            // Verify that compilationItem path relative to its bundle does not begin by exiting the bundle
            // when hotreloading
            let sourceURLToResolve = (try? item.sourceURL.resolvingSymlink()) ?? item.sourceURL
            let itemBundleRelativePath = RelativePath(from: item.bundleInfo.baseDir, to: sourceURLToResolve)
            if let firstPathComponent = itemBundleRelativePath.components.first, firstPathComponent == ".." {
                return false
            }
        }

        if item.sourceURL.lastPathComponent.starts(with: ".") {
            return false
        }

        let path = item.sourceURL.path
        switch item.bundleInfo.inclusionConfig.resolve(path) {
            case .included:
                return true
            case .excluded:
                return false
            case .unknown:
                return true
        }
    }

    func process(items: CompilationItems) throws -> CompilationItems {
        return items.selectAll().transformEachConcurrently { (item) -> [CompilationItem] in
            if resolveInclusion(item: item) {
                return [item]
            }
            return []
        }
    }

}
