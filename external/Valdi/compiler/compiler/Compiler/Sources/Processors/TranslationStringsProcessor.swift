//
//  TranslationStringsProcessor.swift
//
//
//  Created by Simon Corsin on 3/23/20.
//

import Foundation

final class TranslationStringsProcessor: CompilationProcessor {
    
    var description: String {
        return "Processing translation strings"
    }
    
    private let logger: ILogger
    private let fileManager: ValdiFileManager
    private let compilerConfig: CompilerConfig
    private let projectConfig: ValdiProjectConfig
    private let emitInlineTranslations: Bool
    private let companion: CompanionExecutable

    private var cache: Synchronized<[CompilationItem.BundleInfo:[LocalizableStringsContent]]> = Synchronized(data: [:])

    init(logger: ILogger, fileManager: ValdiFileManager, compilerConfig: CompilerConfig, projectConfig: ValdiProjectConfig, emitInlineTranslations: Bool, companion: CompanionExecutable) {
        self.logger = logger
        self.fileManager = fileManager
        self.compilerConfig = compilerConfig
        self.projectConfig = projectConfig
        self.emitInlineTranslations = emitInlineTranslations
        self.companion = companion
    }
    
    private func generateStringsDts(bundleInfo: CompilationItem.BundleInfo, localizableStringContents: [LocalizableStringsContent], defaultLocalizableStringContentsIndex: Int) throws -> [CompilationItem] {
        logger.debug("-- Creating Strings.d.ts for \(bundleInfo.name)")

        do {
            let generator = TypeScriptStringsModuleGenerator(stringFiles: localizableStringContents,
                                                             defaultStringFileIndex: defaultLocalizableStringContentsIndex,
                                                             moduleName: bundleInfo.name,
                                                             emitInlineTranslations: emitInlineTranslations)

            let result = try generator.generate()
            
            let jsOutputPath = TypeScriptStringsModuleGenerator.outputPath(bundleName: bundleInfo.name, ext: FileExtensions.javascript)
            let tsOutputPath = TypeScriptStringsModuleGenerator.outputPath(bundleName: bundleInfo.name, ext: FileExtensions.typescriptDeclaration)

            return try TypeScriptIntermediateUtils.emitGeneratedJs(fileManager: fileManager, projectConfig: projectConfig, bundleInfo: bundleInfo, outputTarget: OutputTarget.all, jsOutputPath: jsOutputPath, jsContent: result.jsFile, tsDefinitionOutputPath: tsOutputPath, tsDefinitionContent: result.tsDefinitionFile)
        } catch let error {
            let errorMessage = "Failed to generate Strings.d.ts for \(bundleInfo.name): \(error.legibleLocalizedDescription)"
            logger.error(errorMessage)
            throw CompilerError(errorMessage)
        }
    }
    
    private func extractAndroidStringsXML(moduleName: String, selectedItem: SelectedItem<File>, response: ExportTranslationStrings.Response) throws -> CompilationItem {
        logger.debug("-- Created Android Strings resource: \(response.outputFileName)")
        
        return selectedItem.item.with(newKind: .androidResource(response.outputFileName, .data(try response.content.utf8Data())), newPlatform: .android)
    }
    
    private func extractIosStrings(moduleName: String, selectedItem: SelectedItem<File>, response: ExportTranslationStrings.Response) throws -> CompilationItem {
        logger.debug("-- Created iOS Strings resource: \(response.outputFileName)")
        
        return selectedItem.item.with(newKind: .iosStringResource(response.outputFileName, .data(try response.content.utf8Data())), newPlatform: .ios)
    }

    private func resolveLocalizableStringsContent(module: CompilationItem.BundleInfo, moduleStrings: [SelectedItem<File>]) -> [LocalizableStringsContent] {
        var localizableStringContents = moduleStrings.map { LocalizableStringsContent(relativePath: $0.item.relativeBundlePath, file: $0.data) }

        if let existingLocalizableStringContents = (self.cache.data { $0[module] }) {
            let asDict = localizableStringContents.associateBy { $0.relativePath }

            for existingLocalizableStringContent in existingLocalizableStringContents {
                if asDict[existingLocalizableStringContent.relativePath] == nil {
                    localizableStringContents.append(existingLocalizableStringContent)
                }
            }
        }

        self.cache.data { $0[module] = localizableStringContents }

        return localizableStringContents
    }

    private func processModuleStrings(moduleStrings: GroupedItems<CompilationItem.BundleInfo, SelectedItem<File>>) throws -> [CompilationItem] {
        let localizableStringsContents = resolveLocalizableStringsContent(module: moduleStrings.key, moduleStrings: moduleStrings.items)
        let moduleName = moduleStrings.key.name
        guard let baseLocaleJSONItemIndex = localizableStringsContents.firstIndex(where: { $0.relativePath.components.last == "strings-en.json" }) else {
            throw CompilerError("Missing strings-en.json for module \(moduleName)")
        }

        var result: [CompilationItem] = []
        let baseLocaleJSONURLHandle: URLHandle

        do {
            baseLocaleJSONURLHandle = try localizableStringsContents[baseLocaleJSONItemIndex].file.getURLHandle()
            result.append(contentsOf: try generateStringsDts(bundleInfo: moduleStrings.key,
                                                             localizableStringContents: localizableStringsContents,
                                                             defaultLocalizableStringContentsIndex: baseLocaleJSONItemIndex))
        } catch let error {
            return moduleStrings.items.map { $0.item.with(error: error) }
        }

        if emitInlineTranslations {
            // string files will be bundled within the .valdimodule
            result.append(contentsOf: moduleStrings.items.map { $0.item.with(newKind: .resourceDocument(DocumentResource(outputFilename: $0.item.relativeBundlePath.description, file: $0.data))) })
        } else {
            let outputForAndroid = compilerConfig.outputForAndroid && self.projectConfig.androidOutput != nil
            let outputForIOS = compilerConfig.outputForIOS && self.projectConfig.iosOutput != nil
            let outputForWeb = compilerConfig.outputForWeb && self.projectConfig.webOutput != nil

            if outputForAndroid || outputForIOS || outputForWeb {
                logger.debug("-- Exporting translation strings for \(moduleName)")

                result.append(contentsOf: try moduleStrings.items
                    .flatMap { selectedItem in
                        var result: [(SelectedItem<File>, Promise<ExportTranslationStrings.Response>, Platform)] = []
                        if outputForAndroid {
                            logger.debug("-- Creating android output for: \(selectedItem.item.sourceURL)")                            
                            result.append((
                                selectedItem,
                                self.companion.exportAndroidStringsTranslations(moduleName: moduleName, baseLocalePath: baseLocaleJSONURLHandle.url.path, inputLocalePath: selectedItem.item.sourceURL.path),
                                Platform.android))

                        }
                        if outputForIOS {
                            logger.debug("-- Creating iOS output for: \(selectedItem.item.sourceURL)")
                            result.append((
                                selectedItem,
                                self.companion.exportIosStringsTranslations(moduleName: moduleName, baseLocalePath: baseLocaleJSONURLHandle.url.path, inputLocalePath: selectedItem.item.sourceURL.path),
                                Platform.ios))
                        }
                        if outputForWeb {
                            logger.debug("-- Creating web output for: \(selectedItem.item.sourceURL)")
                            result.append((selectedItem, Promise(), Platform.web))
                        }

                        return result
                    }
                    .map { (selectedItem: SelectedItem<File>, promise: Promise<ExportTranslationStrings.Response>, platform: Platform) in
                        if platform == Platform.android || platform == Platform.ios || platform == Platform.web {
                            return (selectedItem, try promise.waitForData(), platform)
                        }
                        return (selectedItem, nil, platform)
                    }
                    .map { (selectedItem: SelectedItem<File>, response: ExportTranslationStrings.Response!, platform: Platform) in

                        switch platform {
                            case .android:
                                return try extractAndroidStringsXML(moduleName: moduleName, selectedItem: selectedItem, response: response)
                            case .ios:
                                return try extractIosStrings(moduleName: moduleName, selectedItem: selectedItem, response: response)
                            case .web:
                                // Pass the raw items through for Web
                                return selectedItem.item.with(newPlatform: .web)
                            case .cpp:
                                // Doesn't actually happen yet.
                                return selectedItem.item.with(newPlatform: .cpp)
                        }
                    })
            }
        }
        
        return result
    }
    
    
    func process(items: CompilationItems) throws -> CompilationItems {
        return try items.select { (item) -> File? in
            switch item.kind {
            case .stringsJSON(let file):
                return file
            default:
                return nil
            }
        }.groupBy { selectedItem -> CompilationItem.BundleInfo in
            selectedItem.item.bundleInfo
        }.transformEachConcurrently(processModuleStrings)
    }
}
