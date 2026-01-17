// Copyright © 2024 Snap, Inc. All rights reserved.

extension CompilationItem.BundleInfo {

    private static func makeBazelTarget(currentWorkspace: String?, workspace: String, target: String) -> String {
        if currentWorkspace == workspace {
            return "\(target)"
        } else {
            return "@\(workspace)\(target)"
        }
    }

    func toBazelTarget(projectConfig: ValdiProjectConfig, currentWorkspace: String?) throws -> String {
        let bundleName = self.resolvedBundleName()
        
        let workspace: String
        let target: String
        
        if let openSourceDir = projectConfig.openSourceDir, !openSourceDir.relativePath(to: self.baseDir).hasPrefix("../") {
            workspace = "valdi"
            target = "//src/valdi_modules/src/valdi/\(bundleName)"
        } else if self.baseDir.path.contains("/node_modules/"),
                  let configWorkspace = projectConfig.nodeModulesWorkspace,
                  let configTarget = projectConfig.nodeModulesTarget {
            workspace = configWorkspace
            target = "\(configTarget)/\(bundleName)"
        } else if let configWorkspace = projectConfig.externalModulesWorkspace,
                  let configTarget = projectConfig.externalModulesTarget {
            workspace = configWorkspace
            target = "\(configTarget)/\(bundleName)"
        } else {
            throw CompilerError("No bazel target configuration found for bundle '\(bundleName)' at path: \(self.baseDir.path)")
        }
        
        return CompilationItem.BundleInfo.makeBazelTarget(currentWorkspace: currentWorkspace, workspace: workspace, target: target)
    }
}