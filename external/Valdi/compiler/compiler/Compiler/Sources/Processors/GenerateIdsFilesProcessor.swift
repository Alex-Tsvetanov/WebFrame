//
//  GenerateIdsFilesProcessor.swift
//  
//
//  Created by Simon Corsin on 5/10/22.
//

import Foundation

struct GenerateIdsFilesResult {
    let android: String
    let iosHeader: String
    let iosImplementation: String
    let typescriptDefinition: String
    let typescriptImplementation: String
}

class GenerateIdsFilesProcessor: CompilationProcessor {

    var description: String {
        return "Generating Ids Files"
    }

    private let logger: ILogger
    private let fileManager: ValdiFileManager
    private let projectConfig: ValdiProjectConfig
    private let companion: CompanionExecutable
    private let nativeCodeGenerationManager: NativeCodeGenerationManager

    init(logger: ILogger, fileManager: ValdiFileManager, projectConfig: ValdiProjectConfig, companion: CompanionExecutable, nativeCodeGenerationManager: NativeCodeGenerationManager) {
        self.logger = logger
        self.fileManager = fileManager
        self.projectConfig = projectConfig
        self.companion = companion
        self.nativeCodeGenerationManager = nativeCodeGenerationManager
    }

    private func generateIdFiles(selectedItem: SelectedItem<File>) -> [CompilationItem] {
        do {
            logger.debug("-- Generating Ids files from \(selectedItem.item.relativeProjectPath)")

            let iosFileNamePrefix = "Ids"

            var iosType = IOSType(name: iosFileNamePrefix, bundleInfo: selectedItem.item.bundleInfo, kind: .enum, iosLanguage: selectedItem.item.bundleInfo.iosLanguage)
            iosType.applyImportPrefix(iosImportPrefix: selectedItem.item.bundleInfo.iosModuleName, isOverridden: false)
            let iosHeaderStatement = iosType.importHeaderStatement(kind: .withUtilities)

            let result = try selectedItem.data.withURL { url in
                return try companion.generateIdsFiles(moduleName: selectedItem.item.bundleInfo.name, iosHeaderImportPath: iosHeaderStatement, idFilePath: url.path).waitForData()
            }

            let relativeProjectPathWithoutExtension = selectedItem.item.relativeProjectPath.deletingPathExtension()

            var generatedFiles = try TypeScriptIntermediateUtils.emitGeneratedJs(fileManager: fileManager,
                                                                                 projectConfig: projectConfig,
                                                                                 sourceItem: selectedItem.item,
                                                                                 jsOutputPath: "\(relativeProjectPathWithoutExtension).\(FileExtensions.javascript)",
                                                                                 jsContent: .data(try result.typescriptImplementation.utf8Data()),
                                                                                 tsDefinitionOutputPath: "\(relativeProjectPathWithoutExtension).\(FileExtensions.typescriptDeclaration)",
                                                                                 tsDefinitionContent: .data(try result.typescriptDefinition.utf8Data()))

            generatedFiles.append(selectedItem.item.with(newKind: .nativeSource(NativeSource(relativePath: nil,
                                                                                             filename: "\(iosFileNamePrefix).h",
                                                                                             file: .data(try result.iosHeader.utf8Data()),
                                                                                             groupingIdentifier: "\(selectedItem.item.bundleInfo.iosModuleName).h",
                                                                                             groupingPriority: IOSType.idsTypeGroupingPriority
                                                                                            )
            ), newPlatform: .ios))
            generatedFiles.append(selectedItem.item.with(newKind: .nativeSource(NativeSource(relativePath: nil,
                                                                                             filename: "\(iosFileNamePrefix).m",
                                                                                             file: .data(try result.iosImplementation.utf8Data()),
                                                                                             groupingIdentifier: "\(selectedItem.item.bundleInfo.iosModuleName).m",
                                                                                             groupingPriority: IOSType.idsTypeGroupingPriority)), newPlatform: .ios))

            let androidXmlPath = "values/\(selectedItem.item.bundleInfo.name)_ids.xml"

            generatedFiles.append(selectedItem.item.with(newKind:
                    .androidResource(androidXmlPath, .data(try result.android.utf8Data())), newPlatform: .android))

            return generatedFiles
        } catch let error {
            return [selectedItem.item.with(error: CompilerError("Failed to generate ids files from \(selectedItem.item.relativeProjectPath): \(error.legibleLocalizedDescription)"))]
        }
    }

    func process(items: CompilationItems) throws -> CompilationItems {
        return items.select { (item) -> File? in
            if case .ids(let file) = item.kind {
                return file
            }
            return nil
        }.transformEachConcurrently(generateIdFiles)
    }
}
