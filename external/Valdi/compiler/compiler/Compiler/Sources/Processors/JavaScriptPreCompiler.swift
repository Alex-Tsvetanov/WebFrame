//
//  File.swift
//
//
//  Created by Simon Corsin on 1/24/20.
//

import Foundation

class JavaScriptPreCompilerProcessor: CompilationProcessor {

    var description: String {
        return "Pre-compiling JavaScript files"
    }

    private let logger: ILogger
    private let fileManager: ValdiFileManager
    private let toolboxExecutable: ToolboxExecutable
    private let diskCache: DiskCache?
    private let androidJsBytecodeFormat: String?

    init(logger: ILogger,
         fileManager: ValdiFileManager,
         diskCacheProvider: DiskCacheProvider,
         toolboxExecutable: ToolboxExecutable,
         androidJsBytecodeFormat: String?) throws {
        self.logger = logger
        self.fileManager = fileManager
        self.toolboxExecutable = toolboxExecutable
        self.androidJsBytecodeFormat = androidJsBytecodeFormat

        if diskCacheProvider.isEnabled() {
            let versionString = try toolboxExecutable.getVersionString()
            self.diskCache = diskCacheProvider.newCache(cacheName: "js_precompiler", outputExtension: "jsbin", metadata: [
                "toolbox_version": versionString,
                "android_js_bytecode_format": androidJsBytecodeFormat ?? ""
            ])
        } else {
            self.diskCache = nil
        }

    }

    private func compile(item: SelectedItem<JavaScriptFile>) -> [CompilationItem] {
        var output = [CompilationItem]()
        if item.item.shouldOutputToIOS {
            output.append(item.item.with(newPlatform: .ios))
        }
        if item.item.shouldOutputToWeb {
            output.append(item.item.with(newPlatform: .web))
        }

        // Early out if we're not looking at Android
        if !item.item.shouldOutputToAndroid {
            return output
        }

        if item.item.outputTarget.contains(.debug) {
            output.append(item.item.with(newKind: item.item.kind, newPlatform: .android, newOutputTarget: .debug))
        }

        guard let androidJsBytecodeFormat = self.androidJsBytecodeFormat else {
            output.append(item.item.with(newKind: item.item.kind, newPlatform: .android, newOutputTarget: .release))
            return output
        }

        do {
            let jsPath = item.data.relativePath
            logger.debug("-- Pre-compiling JavaScript file \(jsPath)")

            let data = try item.data.file.readData()

            if let cachedData = diskCache?.getOutput(item: jsPath, platform: .android, target: .release, inputData: data) {
                let outputJsFile = item.data.with(file: .data(cachedData))
                output.append(item.item.with(newKind: .javaScript(outputJsFile), newPlatform: .android))
                return output
            }

            let inputFile = URL.randomFileURL(extension: "js")
            let outputFile = URL.randomFileURL(extension: "js")
            try fileManager.save(data: data, to: inputFile)
            defer {
                _ = try? fileManager.delete(url: inputFile)
                _ = try? fileManager.delete(url: outputFile)
            }

            // Remove .js suffix, JS files are referred without their extension
            // in the consumer code
            var filename = jsPath
            if filename.hasSuffix(".js") {
                filename.removeLast(3)
            }

            try toolboxExecutable.precompile(inputFilePath: inputFile.path, outputFilePath: outputFile.path, filename: filename, engine: androidJsBytecodeFormat)

            let outputData = try File.url(outputFile).readData()

            let outputJsFile = item.data.with(file: .data(outputData))
            try diskCache?.setOutput(item: jsPath, platform: .android, target: .release, inputData: data, outputData: outputData)

            output.append(item.item.with(newKind: .javaScript(outputJsFile), newPlatform: .android))

            return output
        } catch let error {
            logger.error(error.legibleLocalizedDescription)
            return [item.item.with(error: error)]
        }
    }

    func process(items: CompilationItems) throws -> CompilationItems {
        return items.select { (item) -> JavaScriptFile? in
            if !item.shouldOutputToAndroid || item.bundleInfo.compilationModeConfig.resolveCompilationMode(forItemPath: item.relativeProjectPath) == .js || !item.outputTarget.contains(.release) {
                return nil
            }

            switch item.kind {
            case .javaScript(let file):
                return file
            default:
                return nil
            }
        }.transformEachConcurrently(compile)
    }

}
