import Foundation
import Yams

final class GenerateGlobalMetadataProcessor: CompilationProcessor {

    private struct AllModulesAndDepsRecord: Encodable, Decodable {
        let iosModuleName: String
        let deps: [String]
        let ios_context_factories: [String]
    }

    var description: String {
        return "Generating global metadata"
    }

    private let logger: ILogger
    private let projectConfig: ValdiProjectConfig
    private let rootBundle: CompilationItem.BundleInfo
    private let shouldMergeWithExistingFile: Bool

    init(logger: ILogger, projectConfig: ValdiProjectConfig, rootBundle: CompilationItem.BundleInfo, shouldMergeWithExistingFile: Bool) {
        self.logger = logger
        self.projectConfig = projectConfig
        self.rootBundle = rootBundle
        self.shouldMergeWithExistingFile = shouldMergeWithExistingFile
    }

    private func generateAllDepsFile(platform: Platform, moduleYamlFileItems: [SelectedItem<File>]) throws -> [CompilationItem] {
        let output: ValdiOutputConfig?
        switch platform {
        case .ios:
            output = projectConfig.iosOutput
        case .android:
            output = projectConfig.androidOutput
        case .web:
            output = projectConfig.webOutput
        case .cpp:
            output = projectConfig.cppOutput
        }

        guard let globalMetadataURL = try output?.globalMetadataPath?.resolve() else {
            return []
        }

        let outputURL = globalMetadataURL.appendingPathComponent("ALL_MODULES_AND_DEPS.bzl", isDirectory: false)

        // If --module is specified, we want to avoid overwriting the existing ALL_MODULES_AND_DEPS with just the information
        // from the dependency subtree that is currently being compiled. So, in those cases, we take extra steps to parse the
        // existing contents.
        let existingAllModulesAndDeps: [String: AllModulesAndDepsRecord]
        if shouldMergeWithExistingFile && FileManager.default.fileExists(atPath: outputURL.path) {
            do {
                let existingContents = String(data: try Data(contentsOf: outputURL), encoding: .utf8)
                guard let existingJson = existingContents?.dropFirst("ALL_MODULES_AND_DEPS = ".count).data(using: .utf8) else {
                    throw CompilerError("Unexpected contents in existing ALL_MODULES_AND_DEPS.bzl")
                }
                existingAllModulesAndDeps = try .fromJSON(existingJson, keyDecodingStrategy: .convertFromSnakeCase)
            } catch {
                logger.error("Failed to read existing ALL_MODULES_AND_DEPS.bzl: \(error)")
                existingAllModulesAndDeps = [:]
            }
        } else {
            existingAllModulesAndDeps = [:]
        }

        let tuples = try moduleYamlFileItems.map { item -> (String, AllModulesAndDepsRecord) in
            let bundleInfo = item.item.bundleInfo
            let name = try bundleInfo.toBazelTarget(projectConfig: self.projectConfig, currentWorkspace: nil)
            let deps = try bundleInfo.dependencies
                .filter { !$0.isRoot }
                .map { try $0.toBazelTarget(projectConfig: self.projectConfig, currentWorkspace: nil) }
                .uniqueElements()
                .sorted()

            let record = AllModulesAndDepsRecord(iosModuleName: bundleInfo.iosModuleName,
                                                 deps: deps,
                                                 ios_context_factories: bundleInfo.iosGeneratedContextFactories)
            return (name, record)
        }
        let updatedAllModulesAndDeps = try tuples.associateUnique { tuple in
            return tuple
        }
        let allModulesAndDeps = existingAllModulesAndDeps.merging(updatedAllModulesAndDeps, uniquingKeysWith: { _, updated in updated })
        let allModulesAndDepsData = try allModulesAndDeps.toJSON(outputFormatting: [.prettyPrinted, .sortedKeys, .withoutEscapingSlashes], keyEncodingStrategy: .convertToSnakeCase)
        let allModulesAndDepsString = String(data: allModulesAndDepsData, encoding: .utf8) ?? "{}"

        let fileContents = """
ALL_MODULES_AND_DEPS = \(allModulesAndDepsString)
"""

        let file = File.data(try fileContents.utf8Data())
        let finalFile = FinalFile(outputURL: outputURL, file: file, platform: platform, kind: .unknown)

        let projectURL = projectConfig.baseDir.appendingPathComponent(projectConfig.projectName)
        let errorItem = CompilationItem(sourceURL: projectURL,
                                        relativeProjectPath: nil,
                                        kind: .finalFile(finalFile),
                                        bundleInfo: rootBundle,
                                        platform: .android,
                                        outputTarget: .release)
        return [errorItem]
    }

    func process(items: CompilationItems) throws -> CompilationItems {
        return try items.select { (item) -> File? in
            guard case .moduleYaml(let file) = item.kind
            else { return nil }
            return file
        }.transformAll({ selectedItems in
            let android = try generateAllDepsFile(platform: .android, moduleYamlFileItems: selectedItems)
            let ios = try generateAllDepsFile(platform: .ios, moduleYamlFileItems: selectedItems)
            return android + ios + selectedItems.map(\.item)
        })
    }

}
