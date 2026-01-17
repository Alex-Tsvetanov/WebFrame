// Copyright © 2023 Snap, Inc. All rights reserved.

import Foundation

struct DumpedModuleInfo: Codable {
    let path: String
    let name: String
    let dependencies: [String]
}

struct DumpModulesInfoOutput: Codable {
    var modules: [DumpedModuleInfo]
}

struct DumpModulesInfo {

    static func dump(fileManager: ValdiFileManager, configs: ResolvedConfigs, to path: String) throws {
        let url = URL(fileURLWithPath: path)
        let filesFinder = ValdiFilesFinder(url: configs.projectConfig.baseDir)
        let fileUrls = try filesFinder.allFiles()
        let bundleManager = try BundleManager(projectConfig: configs.projectConfig, compilerConfig: configs.compilerConfig, baseDirURLs: [configs.projectConfig.baseDir])

        var allBundleInfos: Set<CompilationItem.BundleInfo> = []
        for fileUrl in fileUrls {
            let bundleInfo = try bundleManager.getBundleInfo(fileURL: fileUrl)
            allBundleInfos.insert(bundleInfo)
        }

        let sortedBundleInfos = allBundleInfos.map { $0 }.sorted(by: { $0.baseDir.path < $1.baseDir.path })

        var output = DumpModulesInfoOutput(modules: [])

        for bundleInfo in sortedBundleInfos where !bundleInfo.isRoot {
            output.modules.append(DumpedModuleInfo(path: bundleInfo.baseDir.path, name: bundleInfo.name, dependencies: bundleInfo.allDependenciesNames.map { $0 }))
        }

        let json = try output.toJSON(outputFormatting: [.prettyPrinted, .sortedKeys])

        try fileManager.save(data: json, to: url)
    }

}
