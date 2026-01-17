import Foundation

class CodeCoverageProcessor: CompilationProcessor {
    private let logger: ILogger
    private let projectConfig: ValdiProjectConfig
    private let jsCodeInstrumentation: JSCodeInstrumentation
    private var typeScriptCompilerManager: TypeScriptCompilerManager
    private let ignoredTestName = try! NSRegularExpression(pattern: "^.*test.*")
    private let includedFileExtensions = try! NSRegularExpression(pattern: "^.*(tsx|ts)$")
    private let ignoredFilesPattern = try! NSRegularExpression(pattern: "^.*\\.(spec.(tsx|ts)|d.ts|vue.ts)")
    private let codeCoverageByPath: Synchronized<[String: Any]> = Synchronized(data: [:])
    private let fileManager: ValdiFileManager

    var description: String {
        return "Code Coverage Processor"
    }

    init(logger: ILogger,
         jsCodeInstrumentation: JSCodeInstrumentation, diskCacheProvider: DiskCacheProvider, projectConfig: ValdiProjectConfig, typeScriptCompilerManager: TypeScriptCompilerManager, fileManager: ValdiFileManager) {
        self.logger = logger
        self.projectConfig = projectConfig
        self.jsCodeInstrumentation = jsCodeInstrumentation
        self.typeScriptCompilerManager = typeScriptCompilerManager
        self.fileManager = fileManager
    }

    private func filterFileForCodeInstrumentation(_ item: SelectedItem<JavaScriptFile>) -> Bool {
        let filePath = item.item.sourceURL.path
        return !item.item.bundleInfo.disableCodeCoverage &&
        !filePath.contains("/res.ts") &&
        !filePath.matches(regex: self.ignoredFilesPattern) &&
        !filePath.matches(regex: self.ignoredTestName) &&
        filePath.matches(regex: self.includedFileExtensions)
    }

    private func addCodeInstrumentation(items: [SelectedItem<JavaScriptFile>]) throws -> [CompilationItem] {
        let inputFiles = try items
            .filter { (item) in filterFileForCodeInstrumentation(item) }
            .parallelMap { (item) in
                logger.info("-- Adding Code Instrumentation to \(item.data.relativePath)")
                return InstrumentationFileConfig(sourceFilePath: item.item.sourceURL.path, fileContent: try item.data.file.readString())
            }

        let codeInstrumentationResults = try self.jsCodeInstrumentation.instrumentFiles(files: inputFiles).waitForData()

        let codeInstrumentationResultsByFileURL = try codeInstrumentationResults.associateUnique { ($0.sourceFilePath, $0) }

        return items.parallelMap { item in
            let compilationItem = item.item

            guard let codeInstrumentationResult = codeInstrumentationResultsByFileURL[item.item.sourceURL.path] else {
                return compilationItem
            }

            logger.info("-- Added Code Instrumentation to \(item.data.relativePath)")

            do {
                let codeCoverage = try JSONSerialization.jsonObject(with: try codeInstrumentationResult.fileCoverage.utf8Data())
                let sourceURLPath = item.item.sourceURL.path
                codeCoverageByPath.data { dict -> Void in
                    dict[sourceURLPath] = codeCoverage
                }

                let outputJs = item.data.with(file: .string(codeInstrumentationResult.instrumentedFileContent))
                return compilationItem.with(newKind: .javaScript(outputJs))
            } catch let error {
                logger.error("Failed to add code coverage to \(item.data.relativePath): \(error.legibleLocalizedDescription)")
                return compilationItem.with(error: error)
            }
        }
    }

    private func addCodeInstrumentationInBatch(items: [SelectedItem<JavaScriptFile>]) -> [CompilationItem] {
        do {
            var outItems: [CompilationItem] = []
            let codeInstrumentedItems = try addCodeInstrumentation(items: items)

            for item in codeInstrumentedItems {
                switch item.kind {
                case .javaScript:
                    outItems.append(item.with(newOutputTarget: .debug))
                default:
                    outItems.append(item)
                }
            }

            return outItems
        } catch {
            return items.map { $0.item.with(error: error) }
        }
    }

    private func generateCodeCoverageIndexFile() throws {
        let allCodeCoverageByPath = codeCoverageByPath.data { data in
            return data
        }

        let entries = allCodeCoverageByPath.sorted { left, right in
            return left.key < right.key
        }

        let json: [String: Any] = ["files": entries.map { [$0.key, $0.value] }]

        let outputURL = self.projectConfig.codeCoverageResultConfig.instrumentedFilesResult

        do {
            let data = try JSONSerialization.data(withJSONObject: json)
            try fileManager.save(data: data, to: outputURL)
        } catch let error {
            throw CompilerError("Failed to generate code coverage index file: \(error.legibleLocalizedDescription)")
        }
    }

    func process(items: CompilationItems) throws -> CompilationItems {
        let outItems = items.select { (item) -> JavaScriptFile? in
            switch item.kind {
            case .javaScript(let jsFile):
                return jsFile
            default:
                return nil
            }
        }.transformInBatches(addCodeInstrumentationInBatch)

        try generateCodeCoverageIndexFile()

        return outItems
    }
}
