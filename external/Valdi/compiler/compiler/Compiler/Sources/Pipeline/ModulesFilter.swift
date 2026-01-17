//
//  ModulesFilter.swift
//  
//
//  Created by Simon Corsin on 6/26/20.
//

import Foundation

struct ModulesFilter {

    private let modulesAndDeps: [CompilationItem.BundleInfo]
    private let modulesFilter: [String]
    private let ignoreModuleDeps: Bool

    init(bundleManager: BundleManager, modulesFilter: [String], ignoreModuleDeps: Bool) throws {
        var modules = [CompilationItem.BundleInfo]()
        modules.append(bundleManager.rootBundle)

        for modulePath in modulesFilter {
            let module = try bundleManager.getBundleInfo(forName: modulePath)

            let dependencies = module.allDependencies.filter { !modules.contains($0) }

            modules.append(module)
            modules += dependencies
        }

        self.modulesAndDeps = modules
        self.modulesFilter = modulesFilter
        self.ignoreModuleDeps = ignoreModuleDeps
    }

    func shouldIncludeInInput(bundleInfo: CompilationItem.BundleInfo) -> Bool {
        return modulesAndDeps.contains(bundleInfo)
    }

    func shouldIncludeInOutput(bundleInfo: CompilationItem.BundleInfo) -> Bool {
        if ignoreModuleDeps {
            return modulesFilter.contains(bundleInfo.name)
        } else {
            return true
        }
    }

}
