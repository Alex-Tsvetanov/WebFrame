import Foundation

class PrependWebJSProcessor: CompilationProcessor {
    let logger: ILogger

    // Files are excluded because they run before module resolution is setup
    let excluded_files = [
        "web_renderer/src/ValdiWebRenderer.js", 
        "web_renderer/src/ValdiWebRuntime.js",
        "valdi_core/src/Init.js", 
        "valdi_core/src/ModuleLoader.js"
    ]

    init(logger: ILogger) {
        self.logger = logger
    }

    var description: String {
        return "Modify js files for web"
    }

    func process(items: CompilationItems) throws -> CompilationItems {
        return items.select { (item) -> FinalFile? in
            switch item.kind {
            case let .finalFile(finalFile):
                if let platform = finalFile.platform, platform == .web, finalFile.outputURL.lastPathComponent.hasSuffix(".js") {
                    return finalFile
                }
                return nil
            default:
                return nil
            }
        }.transformEach { selected -> CompilationItem in
            let item = selected.item
            guard case let .finalFile(finalFile) = item.kind else {
                return item
            }

            let finalFileOutput = finalFile.outputURL.relativeString

            for name in excluded_files {
                if finalFileOutput.contains(name) {
                    return item
                }
            }

            let relativePath = item.relativeProjectPath
            var newFile = finalFile.file
            var contents: String? = try? newFile.readString()
            contents = contents?.replacingOccurrences(of: "require(", with: "customRequire(")
            let prefix = "var customRequire = globalThis.moduleLoader.resolveRequire(\"\(relativePath)\");\n"
            if let data = (prefix + (contents ?? "" )).data(using: .utf8) {
                newFile = .data(data)
            }
            return item.with(newKind: .finalFile(FinalFile(outputURL: finalFile.outputURL, file: newFile, platform: .web, kind: finalFile.kind)))
        }
    }
}