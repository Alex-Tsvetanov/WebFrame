// Copyright © 2025 Snap, Inc. All rights reserved.

import Foundation

// [.nativeSource] -> [.nativeSource]
final class CombineNativeSourcesProcessor: CompilationProcessor {

    fileprivate struct GroupingKey: Equatable, Hashable {
        let platform: Platform?
        let groupingIdentifier: String
    }

    fileprivate struct FileAndContent {
        let filename: String
        let content: String
    }

    private let logger: ILogger
    private let compilerConfig: CompilerConfig
    private let projectConfig: ValdiProjectConfig
    private let bundleManager: BundleManager
    private var cachedNativeSourceByModule = Synchronized(data: [CompilationItem.BundleInfo: [SelectedItem<NativeSource>]]())

    init(logger: ILogger, compilerConfig: CompilerConfig, projectConfig: ValdiProjectConfig, bundleManager: BundleManager) {
        self.logger = logger
        self.compilerConfig = compilerConfig
        self.projectConfig = projectConfig
        self.bundleManager = bundleManager
    }

    var description: String {
        return "Combining Native Sources"
    }

    private func collectNativeSources(bundleInfo: CompilationItem.BundleInfo, selectedItems: [SelectedItem<NativeSource>]) -> [SelectedItem<NativeSource>] {
        let existingItems = self.cachedNativeSourceByModule.data { map in
            return map[bundleInfo] ?? []
        }

        var allItems = selectedItems
        let allItemsByPath = allItems.groupBy { item in
            return GroupingKey(platform: item.item.platform, groupingIdentifier: item.data.groupingIdentifier)
        }

        for existingItem in existingItems {
            let groupingKey = GroupingKey(platform: existingItem.item.platform, groupingIdentifier: existingItem.data.groupingIdentifier)
            guard allItemsByPath[groupingKey] == nil else { continue }

            allItems.append(existingItem)
        }

        self.cachedNativeSourceByModule.data { map in
            map[bundleInfo] = allItems
        }

        return allItems
    }

    private func mergeAnySources(files: [FileAndContent]) -> File {
        var data = ""

        for file in files {
            data +=  "//\n// \(file.filename)\n//\n\n"
            data += file.content
            data += "\n"
        }

        return .string(data)
    }

    private func mergeCppHeaders(files: [FileAndContent]) -> File {
        let pragmaOnce = "#pragma once"
        var data = "\(pragmaOnce)\n"

        for file in files {
            data +=  "//\n// \(file.filename)\n//\n\n"

            for line in file.content.split(separator: "\n", omittingEmptySubsequences: false) where line != pragmaOnce {
                data += line
                data += "\n"
            }
            data += "\n"
        }

        return .string(data)
    }

    private func mergeKotlinSources(files: [FileAndContent]) -> File {
        let writer = CodeWriter()
        var headerStatements = Set<String>()
        writer.appendBody("\n")

        // The following reconstructs a .kt file where package and import statements are de-duplicated
        // and place at the top of the file.
        /**
         The following reconstructs a .kt file where package and import statements are de-duplicated and placed at the top of the file.
         For example:
         packge my_package
         import B
         import C

         class ClassA {
         }

         package my_package
         import B

         class Class B {
         }

         Would become:

         package my_package
         import B
         import C

         class ClassA {
         }

         class ClassB {
         }
         */

        for fileAndContent in files {
            var inBody = false
            for line in fileAndContent.content.components(separatedBy: CharacterSet.newlines) {
                if !inBody {
                    if line.hasPrefix("package ") || line.hasPrefix("import ") || line.isEmpty {
                        if !headerStatements.contains(line) {
                            headerStatements.insert(line)
                            writer.appendHeader(line)
                            writer.appendHeader("\n")
                        }
                    } else {
                        inBody = true
                        writer.appendBody("//\n// \(fileAndContent.filename)\n//\n\n")
                    }
                }

                if inBody {
                    writer.appendBody(line)
                    writer.appendBody("\n")
                }
            }
        }

        return .string(writer.content)
    }

    private func doCombineNativeSources(filename: String,
                                        bundleInfo: CompilationItem.BundleInfo,
                                        platform: Platform?,
                                        nativeSources: [SelectedItem<NativeSource>]) -> CompilationItem {
        let sortedNativeSources = nativeSources.sorted { (left, right) -> Bool in
            if left.data.groupingPriority == right.data.groupingPriority {
                return left.data.filename < right.data.filename
            } else {
                return left.data.groupingPriority < right.data.groupingPriority
            }
        }

        var relativePath: String?
        var firstItemSettingRelativePath: CompilationItem?

        var isKotlin = false
        var isCppHeader = false
        var fileAndContentArray = [FileAndContent]()

        for nativeSource in sortedNativeSources {
            do {
                if relativePath != nativeSource.data.relativePath {
                    if let relativePath {
                        return nativeSource.item.with(error: CompilerError("Modules with single_file_codegen enabled must have generated files output to the same directory or package. Found '\(relativePath)' from emitting file '\(firstItemSettingRelativePath?.relativeProjectPath ?? "<null>")' vs '\(nativeSource.data.relativePath ?? "<null>")' from emitting file '\(nativeSource.item.relativeProjectPath)'"))
                    }
                    relativePath = nativeSource.data.relativePath
                    firstItemSettingRelativePath = nativeSource.item
                }
                if nativeSource.data.filename.hasSuffix(".kt") {
                    isKotlin = true
                } else if nativeSource.data.filename.hasSuffix(".hpp") {
                    isCppHeader = true
                }
                let nativeSourceContent = try nativeSource.data.file.readString()
                fileAndContentArray.append(FileAndContent(filename: nativeSource.data.filename, content: nativeSourceContent))
            } catch let error {
                return nativeSource.item.with(error: error)
            }
        }

        let generatedNativeSource: NativeSource
        if isKotlin {
            let file = mergeKotlinSources(files: fileAndContentArray)
            generatedNativeSource = NativeSource(relativePath: nil, filename: filename, file: file, groupingIdentifier: filename, groupingPriority: 0)
        } else if isCppHeader {
            let file = mergeCppHeaders(files: fileAndContentArray)
            generatedNativeSource = NativeSource(relativePath: relativePath, filename: filename, file: file, groupingIdentifier: filename, groupingPriority: 0)
        } else {
            let file = mergeAnySources(files: fileAndContentArray)
            generatedNativeSource = NativeSource(relativePath: relativePath, filename: filename, file: file, groupingIdentifier: filename, groupingPriority: 0)
        }

        return CompilationItem(sourceURL: bundleInfo.baseDir,
                               relativeProjectPath: nil,
                               kind: .nativeSource(generatedNativeSource),
                               bundleInfo: bundleInfo,
                               platform: platform,
                               outputTarget: .all)
    }

    private func makeEmptySource(bundle: CompilationItem.BundleInfo, filename: String, platform: Platform, relativePath: String? = nil) -> CompilationItem {
        let nativeSource = NativeSource(relativePath: relativePath, filename: filename, file: .string(""), groupingIdentifier: filename, groupingPriority: 0)
        return CompilationItem(generatedFromBundleInfo: bundle, kind: .nativeSource(nativeSource), platform: platform, outputTarget: .all)
    }

    private func generateEmptySourcesIfNeeded(bundle: CompilationItem.BundleInfo,
                                              matchedItems: [SelectedItem<NativeSource>]) -> [CompilationItem] {
        var output = [CompilationItem]()
        let hasKotlin = matchedItems.contains(where: { $0.data.filename.hasSuffix(".kt") })
        let hasSwift = matchedItems.contains(where: { $0.data.filename.hasSuffix(".swift") })
        let hasObjectiveC = matchedItems.contains(where: { $0.data.filename.hasSuffix(".m") })
        let hasCpp = matchedItems.contains(where: { $0.data.filename.hasSuffix(".cpp") })

        if bundle.androidCodegenEnabled && !hasKotlin {
            output.append(makeEmptySource(bundle: bundle, filename: "\(bundle.name).kt", platform: .android))
        }

        if bundle.iosCodegenEnabled && bundle.iosLanguage == .objc && !hasObjectiveC {
            do {
                let iosType = IOSType(name: "Empty", bundleInfo: bundle, kind: .class, iosLanguage: .objc)
                let nativeSources = try NativeSource.iosNativeSourcesFromGeneratedCode(GeneratedCode(apiHeader: CodeWriter(),
                                                                                                     apiImpl: CodeWriter(),
                                                                                                     header: CodeWriter(),
                                                                                                     impl: CodeWriter()),
                                                                                       iosType: iosType, bundleInfo: bundle)

                output += nativeSources.map {
                    NativeSource(relativePath: $0.relativePath, filename: $0.groupingIdentifier, file: $0.file, groupingIdentifier: $0.groupingIdentifier, groupingPriority: $0.groupingPriority)
                }
                .map {
                    CompilationItem(generatedFromBundleInfo: bundle, kind: .nativeSource($0), platform: .ios, outputTarget: .all)
                }
            } catch let error {
                output.append(CompilationItem(generatedFromBundleInfo: bundle, kind: .error(error, originalItemKind: .moduleYaml(.url(bundle.baseDir))), platform: .android, outputTarget: .all))
            }
        }

        if bundle.iosCodegenEnabled && bundle.iosLanguage == .swift && !hasSwift {
            output.append(makeEmptySource(bundle: bundle, filename: "\(bundle.iosModuleName).swift", platform: .ios))
        }

        if bundle.cppCodegenEnabled && !hasCpp {
            let relativePath: String
            if let cppImportPathPrefix = projectConfig.cppImportPathPrefix {
                relativePath = "\(cppImportPathPrefix)\(bundle.name)"
            } else {
                relativePath = bundle.name
            }
            output.append(makeEmptySource(bundle: bundle, filename: "\(bundle.name).cpp", platform: .cpp, relativePath: relativePath))
            output.append(makeEmptySource(bundle: bundle, filename: "\(bundle.name).hpp", platform: .cpp, relativePath: relativePath))
        }

        return output
    }


    private func combineNativeSources(selectedItems: [SelectedItem<NativeSource>]) -> [CompilationItem] {
        let selectedItemsByBundleInfo = selectedItems.groupBy { item in
            return item.item.bundleInfo
        }

        var output = [CompilationItem]()

        for (bundleInfo, selectedItems) in selectedItemsByBundleInfo {
            let nativeSources = self.collectNativeSources(bundleInfo: bundleInfo, selectedItems: selectedItems)

            let nativeSourcesByRelativePath = nativeSources.groupBy { nativeSource in
                return GroupingKey(platform: nativeSource.item.platform, groupingIdentifier: nativeSource.data.groupingIdentifier)
            }

            for (groupingKey, nativeSources) in nativeSourcesByRelativePath {
                output.append(doCombineNativeSources(filename: groupingKey.groupingIdentifier,
                                                     bundleInfo: bundleInfo,
                                                     platform: groupingKey.platform,
                                                     nativeSources: nativeSources))
            }
        }

        // Generate dummy empty sources if --only-generate-native-code-for-modules is specified
        // and nothing was generated.
        for moduleName in compilerConfig.onlyGenerateNativeCodeForModules {
            guard let bundle = try? bundleManager.getBundleInfo(forName: moduleName), bundle.singleFileCodegen else { continue }

            let existingItems = selectedItemsByBundleInfo[bundle, default: []]

            output += generateEmptySourcesIfNeeded(bundle: bundle, matchedItems: existingItems)
        }

        return output
    }

    private func shouldProcessItem(item: CompilationItem) -> Bool {
        return shouldProcessBundle(bundle: item.bundleInfo)
    }

    private func shouldProcessBundle(bundle: CompilationItem.BundleInfo) -> Bool {
        guard bundle.singleFileCodegen else { return false }
        guard !compilerConfig.onlyGenerateNativeCodeForModules.isEmpty else {
            return true
        }
        return compilerConfig.onlyGenerateNativeCodeForModules.contains(bundle.name)
    }

    func process(items: CompilationItems) throws -> CompilationItems {
        return items.select { item -> NativeSource? in
            guard case .nativeSource(let nativeSource) = item.kind, shouldProcessItem(item: item) else {
                return nil
            }

            return nativeSource
        }.transformAll(self.combineNativeSources(selectedItems:))
    }
}
