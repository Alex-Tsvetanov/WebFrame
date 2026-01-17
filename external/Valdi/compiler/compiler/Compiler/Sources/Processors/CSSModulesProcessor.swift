//
//  CSSModulesProcessor.swift
//  
//
//  Created by Simon Corsin on 10/29/19.
//

import Foundation

class CSSModulesProcessor: CompilationProcessor {

    var description: String {
        return "Compiling CSS modules"
    }

    private let logger: ILogger
    private let fileManager: ValdiFileManager
    private let projectConfig: ValdiProjectConfig
    private let fileDependenciesManager: FileDependenciesManager
    private let stagingDirectoryURL: URL = URL.randomFileURL()

    init(logger: ILogger, fileManager: ValdiFileManager, projectConfig: ValdiProjectConfig, fileDependenciesManager: FileDependenciesManager) {
        self.logger = logger
        self.fileManager = fileManager
        self.projectConfig = projectConfig
        self.fileDependenciesManager = fileDependenciesManager
    }

    private func transform(item: SelectedItem<File>) -> [CompilationItem] {
        do {
            logger.debug("-- Compiling CSS module \(item.item.relativeProjectPath)")

            let shouldEmitJsModule = item.item.sourceURL.pathExtension != "vue"

            let fileContent = try item.data.readString()

            let compiler = CSSModuleCompiler(logger: logger, fileContent: fileContent, projectBaseDir: stagingDirectoryURL, relativeProjectPath: item.item.relativeProjectPath)
            let result = try compiler.compile()

            fileDependenciesManager.register(file: item.item.sourceURL, dependencies: result.allOpenedFiles, forKey: "CSSModuleProcessor")

            let moduleRelativePath = try item.item.bundleInfo.itemPath(forRelativeProjectPath: result.compiledCSS.outputPath)
            let compiledCSS = item.item.with(newKind: .resourceDocument(DocumentResource(outputFilename: moduleRelativePath, file: result.compiledCSS.file)))

            var out = [compiledCSS]

            if shouldEmitJsModule {
                out += try TypeScriptIntermediateUtils.emitGeneratedJs(fileManager: fileManager, projectConfig: projectConfig, sourceItem: item.item, jsOutputPath: result.jsModule.outputPath, jsContent: result.jsModule.file, tsDefinitionOutputPath: result.typeDefinition.outputPath, tsDefinitionContent: result.typeDefinition.file)
            }

            return out
        } catch let error {
            logger.error("Failed to compile CSS module: \(error.legibleLocalizedDescription)")
            return [item.item.with(error: error)]
        }
    }

    private func updateStagingDirectory(items: [SelectedItem<File>]) throws {
        guard !items.isEmpty else { return }

        for item in items {
            let itemExtension = item.item.sourceURL.pathExtension
            guard itemExtension == FileExtensions.css || itemExtension == FileExtensions.sass else {
                continue
            }

            try fileManager.save(file: item.data, to: stagingDirectoryURL.appendingPathComponent(item.item.relativeProjectPath))
        }
    }

    func process(items: CompilationItems) throws -> CompilationItems {
        // Ignore the existing .module.css since we generate them
        let itemsWithoutCompiledCSSModules = items.select { (item) -> File? in
            switch item.kind {
            case .typeScript(let file, let url):
                let fileName = url.lastPathComponent
                guard fileName.hasSuffix(".module.css.d.ts") else {
                    return nil
                }
                return file
            default:
                return nil
            }
        }.discardSelected()

        try itemsWithoutCompiledCSSModules.select { item -> File? in
            switch item.kind {
            case .css(let file):
                return file
            default:
                return nil
            }
        }.processAll(updateStagingDirectory(items:))

        let selectedItems = itemsWithoutCompiledCSSModules.select { (item) -> File? in
            switch item.kind {
            case .css(let file):
                guard item.sourceURL.lastPathComponent.contains(".module.") || item.sourceURL.pathExtension == "vue" else {
                    return nil
                }
                return file
            default:
                return nil
            }
        }

#if os(Linux)
        // Running this processor concurrently on Linux occasionally causes a segmentation fault without a backtrace,
        // that we haven't been able to trace.
        return selectedItems.transformEach(transform)
#else
        return selectedItems.transformEachConcurrently(transform)
#endif
    }

}
