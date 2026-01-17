//
//  ResolveOutputPathsProcessor.swift
//  Compiler
//
//  Created by Simon Corsin on 7/24/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

import Foundation

struct ResolvedOutDirectories {
    let iosRelease: OutDirectories?
    let iosDebug: OutDirectories?
    let androidRelease: OutDirectories?
    let androidDebug: OutDirectories?
    let webRelease: OutDirectories?
    let webDebug: OutDirectories?
    let cppRelease: OutDirectories?
    let cppDebug: OutDirectories?

    func forEach(platform: Platform, item: CompilationItem, closure: (CompilationItem, OutDirectories) -> Void) {
        switch platform {
        case .ios:
            forEachIOSDirectories(item: item, closure: closure)
        case .android:
            forEachAndroidDirectories(item: item, closure: closure)
        case .web:
            forEachWebDirectories(item: item, closure: closure)
        case .cpp:
            forEachCppDirectories(item: item, closure: closure)
        }
    }

    private func forEachDirectories(item: CompilationItem,
                                    platform: Platform,
                                    releaseDirectories: OutDirectories?,
                                    debugDirectories: OutDirectories?,
                                    closure: (CompilationItem, OutDirectories) -> Void) {
        guard item.shouldOutput(to: platform) else { return }

        if let releaseDirectories = releaseDirectories, item.outputTarget.contains(.release), !item.isTestItem {
            closure(item.with(newOutputTarget: .release), releaseDirectories)
        }
        if let debugDirectories = debugDirectories, item.outputTarget.contains(.debug) {
            closure(item.with(newOutputTarget: .debug), debugDirectories)
        }
    }

    func forEachIOSDirectories(item: CompilationItem, closure: (CompilationItem, OutDirectories) -> Void) {
        forEachDirectories(item: item, platform: .ios, releaseDirectories: iosRelease, debugDirectories: iosDebug, closure: closure)
    }

    func forEachAndroidDirectories(item: CompilationItem, closure: (CompilationItem, OutDirectories) -> Void) {
        forEachDirectories(item: item, platform: .android, releaseDirectories: androidRelease, debugDirectories: androidDebug, closure: closure)
    }

    func forEachWebDirectories(item: CompilationItem, closure: (CompilationItem, OutDirectories) -> Void) {
        forEachDirectories(item: item, platform: .web, releaseDirectories: webRelease, debugDirectories: webDebug, closure: closure)
    }

    func forEachCppDirectories(item: CompilationItem, closure: (CompilationItem, OutDirectories) -> Void) {
        forEachDirectories(item: item, platform: .cpp, releaseDirectories: cppRelease, debugDirectories: cppDebug, closure: closure)
    }
}

class ExportedAndroidResources {

    let bundleInfo: CompilationItem.BundleInfo
    var files = Set<String>()

    init(bundleInfo: CompilationItem.BundleInfo) {
        self.bundleInfo = bundleInfo
    }
}

struct AndroidOutputDirectory: Hashable {
    let url: URL
    let outputTarget: OutputTarget
}

// [.resourceDocument, .processedResourceImage, .nativeSource, .javaScript, .rawResource] -> [.finalFile]
class ResolveOutputPathsProcessor: CompilationProcessor {

    var description: String {
        return "Resolving output paths"
    }

    private let logger: ILogger
    private let projectConfig: ValdiProjectConfig
    private let compilerConfig: CompilerConfig
    private var allAndroidResByFolder = [AndroidOutputDirectory: ExportedAndroidResources]()
    private let allAndroidResByFolderLock = DispatchSemaphore.newLock()
    private let drawableCapturePattern = try! NSRegularExpression(pattern: "^drawable(-[mhx]+dpi)?/(.*$)", options: [])
    private let shouldGenerateKeepXML: Bool

    init(logger: ILogger, projectConfig: ValdiProjectConfig, compilerConfig: CompilerConfig, shouldGenerateKeepXML: Bool) {
        self.logger = logger
        self.projectConfig = projectConfig
        self.compilerConfig = compilerConfig
        self.shouldGenerateKeepXML = shouldGenerateKeepXML
    }

    private func processImage(item: CompilationItem, file: File, imageScale: Double?, newFilename: String?, isRemote: Bool, directories: ResolvedOutDirectories) -> [CompilationItem] {
        let filename = newFilename ?? item.sourceURL.lastPathComponent

        var out = [CompilationItem]()

        directories.forEachIOSDirectories(item: item) { (item, ios) -> Void in
            out.append(item.with(newKind: .finalFile(FinalFile(outputURL: ios.resourcesURL.appendingPathComponent(filename), file: file, platform: .ios, kind: .image(scale: imageScale, isRemote: isRemote)))))
        }

        directories.forEachAndroidDirectories(item: item) { (item, android) -> Void in
            out.append(item.with(newKind: .finalFile(FinalFile(outputURL: android.resourcesURL.appendingPathComponent(filename), file: file, platform: .android, kind: .image(scale: imageScale, isRemote: isRemote)))))

            let outputDirectory = AndroidOutputDirectory(url: android.resourcesURL, outputTarget: item.outputTarget)

            allAndroidResByFolderLock.lock {
                if let androidResources = allAndroidResByFolder[outputDirectory] {
                    androidResources.files.insert(filename)
                } else {
                    let androidResources = ExportedAndroidResources(bundleInfo: item.bundleInfo)
                    androidResources.files.insert(filename)
                    allAndroidResByFolder[outputDirectory] = androidResources
                }
            }
        }

        directories.forEachWebDirectories(item: item) { (item, web) -> Void in
            let webFilePath = item.relativeBundleURL.absoluteString.deletingLastPathComponent()
            let outputURL =  web.resourcesURL.appendingPathComponent(webFilePath).appendingPathComponent(filename)
            out.append(item.with(newKind: .finalFile(FinalFile(outputURL: outputURL, file: file, platform: .web, kind: .image(scale: imageScale, isRemote: isRemote)))))
        }
        
        return out
    }

    private func processAndroidResource(item: CompilationItem, file: File, filePath: String, directories: ResolvedOutDirectories) -> [CompilationItem] {
        var out = [CompilationItem]()

        directories.forEachAndroidDirectories(item: item) { (item, android) -> Void in
            out.append(item.with(newKind: .finalFile(FinalFile(outputURL: android.resourcesURL.appendingPathComponent(filePath), file: file, platform: .android, kind: .unknown))))
        }

        return out
    }
    
    private func processIosStringResource(item: CompilationItem, file: File, filePath: String, directories: ResolvedOutDirectories) -> [CompilationItem] {
        var out = [CompilationItem]()

        directories.forEachIOSDirectories(item: item) { (item, ios) -> Void in
            let outputURL = ios.baseURL.appendingPathComponent("strings").appendingPathComponent(filePath)

            out.append(item.with(newKind: .finalFile(FinalFile(outputURL: outputURL, file: file, platform: .ios, kind: .unknown))))
        }

        return out
    }

    private func processRegularResource(item: CompilationItem, file: File, kind: FinalFile.Kind, newFilename: String?, directories: ResolvedOutDirectories) -> [CompilationItem] {
        let filename = newFilename ?? item.sourceURL.lastPathComponent
        var out = [CompilationItem]()

        directories.forEachIOSDirectories(item: item) { (item, ios) -> Void in
            let outputURL = ios.assetsURL.appendingPathComponent(filename)
            out.append(item.with(newKind: .finalFile(FinalFile(outputURL: outputURL, file: file, platform: .ios, kind: kind))))
        }

        directories.forEachAndroidDirectories(item: item) { (item, android) -> Void in
            let directoryURL = android.assetsURL
            let outputURL: URL
            if !filename.hasPrefix("src/") {
                // Backward compatibility
                outputURL = directoryURL.appendingPathComponent(item.relativeBundleURL.path).deletingLastPathComponent().appendingPathComponent(filename)
            } else {
                outputURL = directoryURL.appendingPathComponent(filename)
            }
            out.append(item.with(newKind: .finalFile(FinalFile(outputURL: outputURL, file: file, platform: .android, kind: kind))))
        } 

        directories.forEachWebDirectories(item: item) { (item, web) -> Void in
            let directoryURL = web.assetsURL
            let outputURL = directoryURL.appendingPathComponent(filename)
            out.append(item.with(newKind: .finalFile(FinalFile(outputURL: outputURL, file: file, platform: .web, kind: kind))))
        }

        return out
    }

    private func processRawResource(item: CompilationItem, file: File, directories: ResolvedOutDirectories) -> [CompilationItem] {
        let filename = item.sourceURL.lastPathComponent

        if filename.hasAnyExtension(FileExtensions.images) {
            return processImage(item: item, file: file, imageScale: nil, newFilename: filename, isRemote: false, directories: directories)
        } else if filename.hasAnyExtension([FileExtensions.javascript, FileExtensions.valdiCss]) {
            return processRegularResource(item: item, file: file, kind: .compiledSource, newFilename: filename, directories: directories)
        } else {
            return processRegularResource(item: item, file: file, kind: .unknown, newFilename: filename, directories: directories)
        }
    }

    private func processNativeSource(item: CompilationItem, nativeSource: NativeSource, directories: ResolvedOutDirectories) -> [CompilationItem] {
        let relativePath = nativeSource.relativePath ?? "."
        let shouldUseNativeSrcURL = nativeSource.filename.hasSuffix(".c") || nativeSource.filename.hasSuffix(".cpp")

        var out = [CompilationItem]()

        directories.forEachAndroidDirectories(item: item) { (item, android) in
            let targetDirectoryURL = shouldUseNativeSrcURL ? android.nativeSrcURL : android.srcURL
            let url = targetDirectoryURL.appendingPathComponent(relativePath).appendingPathComponent(nativeSource.filename)
            logger.verbose("-- Emitting generated Android native source \(relativePath)/\(nativeSource.filename) to \(url)")
            out.append(item.with(newKind: .finalFile(FinalFile(outputURL: url, file: nativeSource.file, platform: .android, kind: .nativeSource))))
        }

        directories.forEachIOSDirectories(item: item) { (item, ios) in
            let targetDirectoryURL = shouldUseNativeSrcURL ? ios.nativeSrcURL : ios.srcURL
            let url = targetDirectoryURL.appendingPathComponent(relativePath).appendingPathComponent(nativeSource.filename)
            logger.verbose("-- Emitting generated iOS native source \(relativePath)/\(nativeSource.filename) to \(url)")
            out.append(item.with(newKind: .finalFile(FinalFile(outputURL: url, file: nativeSource.file, platform: .ios, kind: .nativeSource))))
        }

        directories.forEachCppDirectories(item: item) { item, cpp in
            let targetDirectoryURL = shouldUseNativeSrcURL ? cpp.nativeSrcURL : cpp.srcURL
            let url = targetDirectoryURL.appendingPathComponent(relativePath).appendingPathComponent(nativeSource.filename)
            logger.verbose("-- Emitting generated C++ native source \(relativePath)/\(nativeSource.filename) to \(url)")
            out.append(item.with(newKind: .finalFile(FinalFile(outputURL: url, file: nativeSource.file, platform: .cpp, kind: .nativeSource))))
        }

        return out
    }

    // Dependency Metadata only exists for iOS.
    private func processDependencyMetadata(item: CompilationItem, metadata: DependencyMetadata, directories: ResolvedOutDirectories) -> [CompilationItem] {
        var out = [CompilationItem]()

        let fixedItem = computeFixedItem(item: item, platform: Platform.ios, directories: directories)

        directories.forEach(platform: Platform.ios, item: fixedItem) { (item, outDirectories) -> Void in
            let outputURL = outDirectories.metadataURL?.appendingPathComponent("dependencyData").appendingPathExtension(FileExtensions.json)

            if let outputURL = outputURL {
                out.append(item.with(newKind: .finalFile(
                    FinalFile(outputURL: outputURL,
                              file: metadata.file,
                              platform: Platform.ios,
                              kind: .dependencyInjectionData))))
            }
        }

        return out
    }

    private func processDiagnosticsFile(item: CompilationItem, file: File, directories: ResolvedOutDirectories) -> [CompilationItem] {
        let relativeSourceFilePath = item.bundleInfo.baseDir
            .appendingPathComponent(item.relativeBundleURL.path)
            .relativePath(from: projectConfig.baseDir)
        let outputURL = projectConfig.diagnosticsDir.appendingPathComponent(relativeSourceFilePath).appendingPathExtension(FileExtensions.json)
        let finalFile = FinalFile(outputURL: outputURL, file: file, platform: nil, kind: .unknown)
        return [item.with(newKind: .finalFile(finalFile))]
    }

    private func processCompilationMetadata(item: CompilationItem, file: File, directories: ResolvedOutDirectories) -> [CompilationItem] {
        guard let outputDumpedSymbolsDirectory = compilerConfig.outputDumpedSymbolsDirectory else {
            return []
        }

        let outputURL = outputDumpedSymbolsDirectory
            .appendingPathComponent(item.bundleInfo.name)
            .appendingPathComponent(Files.compilationMetadata)

        let finalFile = FinalFile(outputURL: outputURL, file: file, platform: nil, kind: .unknown)
        return [item.with(newKind: .finalFile(finalFile))]
    }

    private func processJs(item: CompilationItem, jsFile: JavaScriptFile, directories: ResolvedOutDirectories) -> [CompilationItem ] {
        let filename: String

        // Make sure the resouces have a javascript file extension
        if item.sourceURL.pathExtension != FileExtensions.javascript {
            filename = item.relativeBundleURL.deletingPathExtension()
                .appendingPathExtension(FileExtensions.javascript).absoluteString
        } else {
            filename = item.relativeBundleURL.absoluteString
        }

        return processRegularResource(item: item, file: jsFile.file, kind: .compiledSource, newFilename: filename, directories: directories)
    }

    private func processDeclaration(item: CompilationItem, file: JavaScriptFile, directories: ResolvedOutDirectories) -> [CompilationItem ] {
        let filename: String

        // Make sure the resouces have a d.ts file extension
        if !item.sourceURL.absoluteString.hasSuffix(".d.ts") {
            filename = item.relativeBundleURL.deletingPathExtension()
                .appendingPathExtension(FileExtensions.typescriptDeclaration).absoluteString
        } else {
            filename = item.relativeBundleURL.absoluteString
        }

        return processRegularResource(item: item, file: file.file, kind: .compiledSource, newFilename: filename, directories: directories)
    }

    private func processWebOnlyFile(item: CompilationItem, file: File, directories: ResolvedOutDirectories) -> [CompilationItem] {
        // We only need the original build file for web
        let webDirectories = ResolvedOutDirectories(
                iosRelease: nil,
                iosDebug: nil,
                androidRelease: nil,
                androidDebug: nil,
                webRelease: directories.webRelease,
                webDebug: directories.webDebug,
                cppRelease: nil,
                cppDebug: nil
            )

        return processRegularResource(item: item, file: file, kind: .unknown, newFilename: item.relativeBundleURL.absoluteString, directories: webDirectories)
    }

    private func computeFixedItem(item: CompilationItem, platform: Platform, directories: ResolvedOutDirectories) -> CompilationItem {
        let wouldTrample: Bool
        switch platform {
        case .ios:
            wouldTrample = directories.iosDebug?.metadataURL == directories.iosRelease?.metadataURL
        case .android:
            wouldTrample = directories.androidDebug?.metadataURL == directories.androidRelease?.metadataURL
        case .web:
            wouldTrample = directories.webDebug?.metadataURL == directories.webRelease?.metadataURL
        case .cpp:
            wouldTrample = directories.cppDebug?.metadataURL == directories.cppRelease?.metadataURL
        }

        if wouldTrample {
            return item.with(newOutputTarget: .debug)
        }

        return item
    }

    private func generateKeepXML() -> [CompilationItem] {
        guard shouldGenerateKeepXML else {
            return []
        }

        var out = [CompilationItem]()

        allAndroidResByFolderLock.lock {
            for (outputDirectory, androidResources) in allAndroidResByFolder {
                var allFilenames = Set<String>()
                for file in androidResources.files {
                    if let match = drawableCapturePattern.firstMatch(in: file, options: [], range: file.nsrange) {
                        let range = match.range(at: 2)
                        let filename = file.substring(with: range)!
                        let filenameWithoutExtension = String(filename.split(separator: ".").first!)
                        allFilenames.insert(filenameWithoutExtension)
                    }
                }

                guard !allFilenames.isEmpty else {
                    continue
                }

                let keepFileStr = allFilenames.sorted().map { "@drawable/\($0)" }.joined(separator: ",")

                let xmlFile = try! """
                <?xml version="1.0" encoding="utf-8"?>
                <!-- These resources are fetched using reflection from valdi -->
                <resources xmlns:tools="http://schemas.android.com/tools" tools:keep="\(keepFileStr)" tools:shrinkMode="strict" />
                """.utf8Data()
                let outputURL = outputDirectory.url.appendingPathComponent("raw/valdi_\(androidResources.bundleInfo.name)_keep.xml", isDirectory: false)

                out.append(CompilationItem(sourceURL: androidResources.bundleInfo.baseDir, relativeProjectPath: nil, kind: .finalFile(FinalFile(outputURL: outputURL, file: .data(xmlFile), platform: .android, kind: .unknown)), bundleInfo: androidResources.bundleInfo, platform: .android, outputTarget: outputDirectory.outputTarget))
            }
        }

        return out
    }

    func process(items: CompilationItems) throws -> CompilationItems {
        let items = items.selectAll().transformEachConcurrently { (item) -> [CompilationItem] in
            let directories = ResolvedOutDirectories(
                iosRelease: item.bundleInfo.iosReleaseOutputDirectories,
                iosDebug: item.bundleInfo.iosDebugOutputDirectories,
                androidRelease: item.bundleInfo.androidReleaseOutputDirectories,
                androidDebug: item.bundleInfo.androidDebugOutputDirectories,
                webRelease: item.bundleInfo.webReleaseOutputDirectories,
                webDebug: item.bundleInfo.webDebugOutputDirectories,
                cppRelease: item.bundleInfo.cppReleaseOutputDirectories,
                cppDebug: item.bundleInfo.cppDebugOutputDirectories
            )

            switch item.kind {
            case .rawResource(let file):
                return processRawResource(item: item, file: file, directories: directories)
            case .resourceDocument(let documentResource):
                return processRegularResource(item: item, file: documentResource.file, kind: .compiledSource, newFilename: documentResource.outputFilename, directories: directories)
            case .processedResourceImage(let imageResource):
                return processImage(item: item, file: imageResource.file, imageScale: imageResource.imageScale, newFilename: imageResource.outputFilename, isRemote: imageResource.isRemote, directories: directories)
            case .androidResource(let filePath, let file):
                return processAndroidResource(item: item, file: file, filePath: filePath, directories: directories)
            case .unknown:
                logger.warn("Ignoring unknown file \(item.sourceURL.path)")
                return []
            case .nativeSource(let nativeSource):
                return processNativeSource(item: item, nativeSource: nativeSource, directories: directories)
            case .finalFile:
                return [item]
            case .javaScript(let file):
                return processJs(item: item, jsFile: file, directories: directories)
            case .declaration(let file):
                return processDeclaration(item: item, file: file, directories: directories)
            case .dependencyMetadata(let metadata):
                return processDependencyMetadata(item: item, metadata: metadata, directories: directories)
            case .diagnosticsFile(let file):
                return processDiagnosticsFile(item: item, file: file, directories: directories)
            case .compilationMetadata(let file):
                return processCompilationMetadata(item: item, file: file, directories: directories)
            case .downloadableArtifactSignatures:
                return [item]
            case .iosStringResource(let filePath, let file):
                return processIosStringResource(item: item, file: file, filePath: filePath, directories: directories)
            case .buildFile(let file), .stringsJSON(let file):
                return processWebOnlyFile(item: item, file: file, directories: directories)
            case .document(let result):
                if (result.componentPath.fileName.contains(".vue")) {
                    
                    return processWebOnlyFile(item: item, file: .string(result.originalDocument.content), directories: directories)
                } else {
                    return []
                }
            case .rawDocument,
            .parsedDocument,
            .document,
            .exportedType,
            .kotlin,
            .generatedTypeDescription,
            .moduleYaml,
            .imageAsset,
            .classMapping,
            .typeScript,
            .css,
            .ids,
            .sql,
            .sqlManifest,
            .error,
            .exportedDependencyMetadata,
            .dumpedTypeScriptSymbols:
                return []
            }
        }

        let keepXMLItems = generateKeepXML()

        return CompilationItems(compileSequence: items.compileSequence, items: items.allItems + keepXMLItems)
    }

}
