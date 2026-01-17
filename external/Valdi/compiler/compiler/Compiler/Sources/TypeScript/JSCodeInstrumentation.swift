import Foundation

struct InstrumentationFileConfig: Codable {
    let sourceFilePath: String
    let fileContent: String
}

struct CodeInstrumentationResult: Codable {
    let sourceFilePath: String
    let instrumentedFileContent: String
    let fileCoverage: String
}

class JSCodeInstrumentation {
    private let companion: CompanionExecutable

    init(companion: CompanionExecutable) {
        self.companion = companion
    }

    func instrumentFiles(files: [InstrumentationFileConfig]) -> Promise<[CodeInstrumentationResult]> {
        return companion.instrumentJSFiles(files: files)
    }
}
