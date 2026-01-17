//
//  FinalFilesVerificationProcessor.swift
//  Compiler
//
//  Created by saniul on 25/07/2019.
//

import Foundation

class FinalFilesVerificationProcessor: CompilationProcessor {

    private let logger: ILogger
    let projectConfig: ValdiProjectConfig

    init(logger: ILogger, projectConfig: ValdiProjectConfig) {
        self.logger = logger
        self.projectConfig = projectConfig
    }

    var description: String {
        return "Verifying final files before saving"
    }

    func process(items: CompilationItems) throws -> CompilationItems {
        var alreadyEncountered: [URL: [CompilationItem]] = [:]
        var urlsWithMultipleFinalFiles: Set<URL> = []

        for item in items.allItems {
            guard case let .finalFile(finalFile) = item.kind else {
                throw CompilerError("Final verification failed: Only .finalFiles are expected")
            }

            if alreadyEncountered.keys.contains(finalFile.outputURL) {
                urlsWithMultipleFinalFiles.insert(finalFile.outputURL)
            }
            alreadyEncountered[finalFile.outputURL] = alreadyEncountered[finalFile.outputURL, default: []] + [item]
        }

        guard !urlsWithMultipleFinalFiles.isEmpty else {
            // All good

            return items
        }

        logger.error("🚨 Multiple .finalFile items writing to the same output URL 🚨")
        logger.error("(This may mean that you have multiple annotated TypeScript types generating the same native type)")
        for (urlIndex, urlWithDupes) in urlsWithMultipleFinalFiles.enumerated() {
            logger.error("\(urlIndex+1). Output URL: \(urlWithDupes.relativePath(from: projectConfig.baseDir)):")
            let items = alreadyEncountered[urlWithDupes, default: []]

            for item in items {
                logger.error("-- Source URL: \(item.sourceURL.relativePath(from: projectConfig.baseDir)), Output platform: \(item.platform?.rawValue ?? "<none>"), Output target: \(item.outputTarget.description)")
            }
        }
        throw CompilerError("Multiple final files writing to the same output URL")
    }
}
