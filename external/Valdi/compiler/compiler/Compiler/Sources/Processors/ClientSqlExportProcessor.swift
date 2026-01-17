//
//  ClientSqlExportProcessor.swift
//
//
//  Created by Alex Pawlowski on 10/02/23.
//

import Foundation
import Yams

final class ClientSqlExportProcessor: CompilationProcessor {
    private let logger: ILogger
    private let fileManager: ValdiFileManager
    private let projectConfig: ValdiProjectConfig
    private let rootBundle: CompilationItem.BundleInfo

    var description: String {
        return "Exporting SQL files"
    }

    init(logger: ILogger,
         fileManager: ValdiFileManager,
         projectConfig: ValdiProjectConfig,
         rootBundle: CompilationItem.BundleInfo
    ) {
        self.logger = logger
        self.fileManager = fileManager
        self.projectConfig = projectConfig
        self.rootBundle = rootBundle
    }

    private func getSqlMigrations(sqlBaseUrl: URL) throws -> [URL] {
        let filesFinder = ValdiFilesFinder(url: sqlBaseUrl)
        let allFiles = try filesFinder.allFiles()
        let steps = allFiles.filter { $0.pathExtension == "sqm" }
        return steps
    }

    private func exportSqlFilesAndroid(
        bundle: CompilationItem.BundleInfo,
        manifestUrl: URL
    ) throws -> [CompilationItem] {
        let platform: Platform = .android

        guard let globalMetadataURL = try projectConfig.globalMetadataURL(for: platform) else {
            return []
        }

        let manifestContent = try String(contentsOf: manifestUrl)

        guard let manifest = try Yams.compose(yaml: manifestContent) else {
            throw CompilerError("Failed to parse sql manifest at: \(manifestUrl)")
        }

        // create the output dir
        let sqldelightUrl = globalMetadataURL.appendingPathComponent(Files.sqlExportDir, isDirectory: true)
        let outputUrl = URL(fileURLWithPath: bundle.name, isDirectory: true, relativeTo: sqldelightUrl)

        let sqlBaseUrl = manifestUrl.deletingLastPathComponent()

        logger.verbose("-- CLIENTSQL exporting sql source files in \(manifestUrl.relativePath(from: projectConfig.baseDir))")

        // for each file under manifest.files
        var exportedFiles: [CompilationItem] = (manifest["files"]?.sequence ?? [])
            .compactMap { $0.string }
            .compactMap { filename in
                let sqlSrc = URL(fileURLWithPath: filename, isDirectory: false, relativeTo: sqlBaseUrl)
                let sqlDst = URL(fileURLWithPath: filename, isDirectory: false, relativeTo: outputUrl)
                if sqlSrc.pathExtension == "sqm" {
                    logger.verbose("-- CLIENTSQL skipping migration file: \(filename)")
                    return nil
                }

                logger.verbose("-- CLIENTSQL - \(filename)")

                let file = File.url(sqlSrc)
                let finalFile = FinalFile(outputURL: sqlDst, file: file, platform: platform, kind: .unknown)

                return CompilationItem(sourceURL: sqlDst,
                                       relativeProjectPath: nil,
                                       kind: .finalFile(finalFile),
                                       bundleInfo: rootBundle,
                                       platform: platform,
                                       outputTarget: .all)
            }

        let migrationFiles = try getSqlMigrations(sqlBaseUrl: sqlBaseUrl)
        try migrationFiles.forEach { sqm in
            let sqmFile = File.data(try Data(contentsOf: sqm))
            let outputUrl = URL(fileURLWithPath: sqm.lastPathComponent, isDirectory: false, relativeTo: outputUrl)
            let finalFile = FinalFile(outputURL: outputUrl, file: sqmFile, platform: platform, kind: .unknown)
            exportedFiles += [CompilationItem(sourceURL: manifestUrl,
                                              relativeProjectPath: nil,
                                              kind: .finalFile(finalFile),
                                              bundleInfo: rootBundle,
                                              platform: platform,
                                              outputTarget: .all)]
        }

        if !exportedFiles.isEmpty {
            // generate BUILD.bazel for Android
            let bazelUrl = URL(fileURLWithPath: "BUILD.bazel", isDirectory: false, relativeTo: outputUrl)
            let bazelContent = """
            load("@rules_pkg//:mappings.bzl", "pkg_files", "strip_prefix")
            load("@rules_pkg//:pkg.bzl", "pkg_zip")

            pkg_files(
                name = "srcs",
                srcs = glob(["**/*.sq", "**/*.sqm", "**/*.yaml"]),
                strip_prefix = strip_prefix.from_pkg(),
            )

            pkg_zip(
                name = "\(bundle.name)_sql",
                srcs = [":srcs"],
                visibility = ["//visibility:public"],
                package_dir = "com/snap",
            )
            """
            let bazelFile = File.data(try bazelContent.utf8Data())
            let finalFile = FinalFile(outputURL: bazelUrl, file: bazelFile, platform: platform, kind: .unknown)
            exportedFiles += [CompilationItem(sourceURL: manifestUrl,
                                              relativeProjectPath: nil,
                                              kind: .finalFile(finalFile),
                                              bundleInfo: rootBundle,
                                              platform: platform,
                                              outputTarget: .all)]
        }

        return exportedFiles
    }

    private func generateExportIndexAndroid(manifestYamlFileItems: [SelectedItem<File>]) throws -> [CompilationItem] {
        guard let globalMetadataURL = try projectConfig.globalMetadataURL(for: .android) else {
            return []
        }
        
        guard let pathTemplate = projectConfig.sqlExportPathTemplate else {
            logger.error("Missing required config 'sql_export_path_template' in valdi_config.yaml. Skipping SQL export index generation.")
            return []
        }
        
        let bundlesWithExports = manifestYamlFileItems.map({
            $0.item.bundleInfo.name
        })
        // TODO(3521): Update to valid_modules
        let labels = Array(bundlesWithExports.map({bundleName in
            pathTemplate.replacingOccurrences(of: "{bundle_name}", with: bundleName)
        }))
        let labelsJson = try labels.toJSON(outputFormatting: [.sortedKeys, .prettyPrinted, .withoutEscapingSlashes])
        let labelsString = String(data: labelsJson, encoding: .utf8) ?? "[]"
        let exportIndexUrl = URL(fileURLWithPath: "sqldelight/BUILD.bazel", isDirectory: false, relativeTo: globalMetadataURL)
        let exportIndexContent = """
        load("@rules_pkg//:pkg.bzl", "pkg_zip")

        pkg_zip(
            name = "all_sql_exports",
            srcs = \(labelsString),
            visibility = ["//visibility:public"],
        )
        """
        let exportIndexFile = File.data(try exportIndexContent.utf8Data())
        let finalFile = FinalFile(outputURL: exportIndexUrl, file: exportIndexFile, platform: .android, kind: .unknown)
        let projectURL = projectConfig.baseDir.appendingPathComponent(projectConfig.projectName)

        return [CompilationItem(sourceURL: projectURL,
                                relativeProjectPath: nil,
                                kind: .finalFile(finalFile),
                                bundleInfo: rootBundle,
                                platform: .android,
                                outputTarget: .all)]
    }

    func process(items: CompilationItems) throws -> CompilationItems {
        return try items.select { item -> File? in
            switch item.kind {
            case .sqlManifest(let file):
                return file
            default:
                return nil
            }
        }
        .transformAll { selectedItems in
            let exportAndroid = (projectConfig.androidOutput?.codegenEnabled ?? false) ? try selectedItems.flatMap {
                try exportSqlFilesAndroid(
                    bundle: $0.item.bundleInfo,
                    manifestUrl: $0.data.getURLHandle().url
                )
            } : []
            let androidIndex = try generateExportIndexAndroid(manifestYamlFileItems: selectedItems)

            return exportAndroid + androidIndex
        }
    }
}
