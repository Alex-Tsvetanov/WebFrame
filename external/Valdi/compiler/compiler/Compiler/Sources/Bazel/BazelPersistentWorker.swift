// Copyright © 2024 Snap, Inc. All rights reserved.

import Foundation

fileprivate extension Data {
    mutating func appendNewline() {
        var newline: UInt8 = 10
        Swift.withUnsafePointer(to: &newline) {
            append($0, count: 1)
        }
    }
}

class BazelPersistentWorker {

    struct Request {
        let arguments: [String]
        let logger: ILogger
        let cancelableToken: StateCancelable
        let workingDirectory: String
    }

    struct Response {
        let result: Promise<Void>
    }

    typealias StartHandler = (Request) -> Response

    private struct CurrentRequest {
        let cancelableToken: StateCancelable
    }

    private let stdin: FileHandleReader
    private let stdout: FileHandle
    private let workQueue = DispatchQueue(label: "com.snap.valdi.compiler.BazelWorkQueue", qos: .userInitiated)
    private let logger: ILogger
    private var requestById = [Int: CurrentRequest]()

    init(stdin: FileHandle, stdout: FileHandle, logger: ILogger) {
        self.stdin = FileHandleReader(fileHandle: stdin, dispatchQueue: DispatchQueue.main)
        self.stdout = stdout
        self.logger = logger
    }

    func run(startHandler: @escaping StartHandler) {
        self.stdin.onDidReceiveData = { [weak self] reader in
            guard let self else { return }
            while self.consumeNextRequest(startHandler: startHandler) {}
        }
        RunLoop.main.run()
    }

    private func submitResponse(exitCode: Int, output: String, requestId: Int, wasCancelled: Bool) {
        let workResponse = BazelWorkerProtocol.WorkResponse(exitCode: exitCode, output: output, requestId: requestId, wasCancelled: wasCancelled)

        do {
            var json = try workResponse.toJSON(keyEncodingStrategy: .useDefaultKeys)
            json.appendNewline()

            try self.stdout.write(contentsOf: json)
        } catch let error {
            logger.error("Failed to write Bazel response: \(error.legibleLocalizedDescription)")
            logger.flush()
            _exit(1)
        }
    }

    private func onRequestCompleted(result: Result<Void, Error>, logOutput: BufferLoggerOutput, requestId: Int) {
        guard requestById.removeValue(forKey: requestId) != nil else {
            return
        }

        var logData = logOutput.data
        var exitCode: Int32

        switch result {
        case .success:
            exitCode = EXIT_SUCCESS
        case .failure(let error):
            exitCode = EXIT_FAILURE
            if let errorData = try? error.legibleLocalizedDescription.utf8Data() {
                logData.appendNewline()
                logData.append(errorData)
            }
        }
        submitResponse(exitCode: Int(exitCode), output: String(data: logData, encoding: .utf8) ?? "", requestId: requestId, wasCancelled: false)
    }

    private func process(data: Data, startHandler: @escaping StartHandler) {
        do {
            let workRequest = try BazelWorkerProtocol.WorkRequest.fromJSON(data, keyDecodingStrategy: .useDefaultKeys)
            let requestId = workRequest.requestId ?? 0

            if let cancel = workRequest.cancel, cancel {
                if let cancelToken = self.requestById.removeValue(forKey: requestId) {
                    cancelToken.cancelableToken.cancel()
                    self.submitResponse(exitCode: 0, output: "", requestId: requestId, wasCancelled: true)
                }

                return
            }

            let cancelableToken = StateCancelable()

            self.requestById[requestId] = CurrentRequest(cancelableToken: cancelableToken)

            workQueue.async {
                let logOutput = BufferLoggerOutput()
                let logger = Logger(output: logOutput)
                let response = startHandler(Request(
                    arguments: workRequest.arguments,
                    logger: logger,
                    cancelableToken: cancelableToken,
                    workingDirectory: workRequest.sandboxDir ?? FileManager.default.currentDirectoryPath))
                response.result.onComplete { result in
                    logger.flush()
                    DispatchQueue.main.async {
                        self.onRequestCompleted(result: result, logOutput: logOutput, requestId: requestId)
                    }
                }
            }
        } catch let error {
            let inputData = String(data: data, encoding: .utf8) ?? ""
            logger.error("Failed to process Bazel request: \(error.legibleLocalizedDescription) (request content: \(inputData)")
            logger.flush()
            _exit(1)
        }
    }

    private func consumeNextRequest(startHandler: @escaping StartHandler) -> Bool {
        guard let newLineIndex = stdin.content.fastFirstIndex(of: /* newline separator */ 10) else {
            return false
        }

        process(data: stdin.content[..<newLineIndex], startHandler: startHandler)
        stdin.trimContent(by: newLineIndex + 1)

        return true
    }
}
