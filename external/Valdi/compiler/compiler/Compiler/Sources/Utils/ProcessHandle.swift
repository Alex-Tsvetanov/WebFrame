//
//  ProcessHandle.swift
//  Compiler
//
//  Created by Simon Corsin on 4/12/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

import Foundation
import Dispatch

class ProcessHandle: NSObject {
    private let logger: ILogger

    var exitedSuccesfully: Bool {
        // process.terminationStatus "raises an NSInvalidArgumentException if the receiver is still running.
        // Verify that the receiver isn’t running before you use it."
        guard !process.isRunning else {
            logger.error("Checking exitedSuccessfully for process \(process.processIdentifier), but it's still running")
            return false
        }

        return terminationStatus == 0
    }

    fileprivate var terminationStatus: Int32 {
        if let cachedTerminationStatus = self.cachedTerminationStatus.data({ $0 }) {
            return cachedTerminationStatus
        }

        // We've seen cases where accessing process.terminationStatus causes signal 4 (SIGILL).
        // This seemed to occur on the second time we're accessing process.terminationStatus of a SyncProcessHandle
        // after its waitUntilExit() has completed.
        //
        // So, let's try to counteract that by caching the termination status and only reading it out of the Process once.
        let cachedTerminationStatus = process.terminationStatus
        self.cachedTerminationStatus.data { $0 = cachedTerminationStatus }
        return cachedTerminationStatus
    }

    var arguments: [String]? {
        get { return process.arguments }
        set { process.arguments = newValue }
    }

    var isRunning: Bool {
        return process.isRunning
    }
    var stdinFileHandle: FileHandle {
        return stdin.fileHandleForWriting
    }

    var environment: [String: String]? {
        get { return process.environment }
        set { process.environment = newValue }
    }

    let stdout: PipeHandle
    let stderr: PipeHandle
    let url: URL
    let dispatchQueue: DispatchQueue?

    private let stdin = Pipe()

    fileprivate let process: Process

    private var cachedTerminationStatus = Synchronized<Int32?>(data: nil)

    fileprivate init(logger: ILogger, url: URL, dispatchQueue: DispatchQueue?) {
        self.logger = logger
        self.url = url
        self.dispatchQueue = dispatchQueue

        stdout = PipeHandle(dispatchQueue: dispatchQueue)
        stderr = PipeHandle(dispatchQueue: dispatchQueue)

        process = Process()
        process.executableURL = url
        process.environment = ProcessInfo.processInfo.environment
        process.standardOutput = stdout.pipe
        process.standardError = stderr.pipe
        process.standardInput = stdin

        super.init()
    }

    private func cancel() {
        cleanUp()
        process.terminate()
    }

    fileprivate func runUnderlyingProcess() throws {
        if #available(OSX 10.13, *) {
            try process.run()
        } else {
            process.launch()
        }
    }

    private func ensureFileIsExecutable() {
        do {
            let attributes = try FileManager.default.attributesOfItem(atPath: url.path)
            if let permissions = attributes[FileAttributeKey.posixPermissions] as? Int {
                let executableMask = 0b001001001
                if permissions & executableMask != executableMask {
                    try FileManager.default.setAttributes([FileAttributeKey.posixPermissions: permissions | executableMask], ofItemAtPath: url.path)
                }
            }
        } catch let error {
            logger.error("Unable to get attribute of executable at \(url.path): \(error)")
        }
    }

    fileprivate func prepareForRun() {
        ensureFileIsExecutable()
    }

    fileprivate func cleanUp() {
        process.terminationHandler = nil
    }

    class Error: CompilerError {
        init(commandName: String, arguments: [String]?, output: String) {
            self.output = output
            super.init("Command \(commandName) with arguments \(arguments ?? []) failed: \(output)")
        }
        let output: String
    }
}

class AsyncProcessHandle: ProcessHandle {

    init(logger: ILogger, url: URL, dispatchQueue: DispatchQueue, cwd: String?, envOverrides: [String: String]? = nil) {
        super.init(logger: logger, url: url, dispatchQueue: dispatchQueue)
        if let cwd = cwd {
            process.currentDirectoryURL = URL(fileURLWithPath: cwd)
            process.environment = envOverrides
        }
    }

    func runAsync(completion: @escaping (ProcessHandle) -> Void) throws {
        prepareForRun()

        process.terminationHandler = { _ in
            // Keep retain cycle while the process is running
            self.cleanUp()
            completion(self)
        }
        try super.runUnderlyingProcess()
    }

    func run() -> Promise<Void> {
        let promise = Promise<Void>()

        do {
            try runAsync { _ in
                promise.resolve(data: Void())
            }
        } catch let error {
            promise.reject(error: error)
        }

        return promise
    }

    class func usingEnv(logger: ILogger, command: [String], dispatchQueue: DispatchQueue, cwd: String? = nil, envOverrides: [String: String]? = nil) -> AsyncProcessHandle {
        let processHandle = AsyncProcessHandle(logger: logger,
                                               url: URL(fileURLWithPath: "/usr/bin/env"),
                                               dispatchQueue: dispatchQueue,
                                               cwd: cwd,
                                               envOverrides: envOverrides)
        processHandle.arguments = command

        return processHandle
    }
}

class SyncProcessHandle: ProcessHandle {

    init(logger: ILogger, url: URL) {
        super.init(logger: logger, url: url, dispatchQueue: nil)
    }

    class func run(logger: ILogger, command: String, arguments: [String]) throws -> String {
        return try run(logger: logger, url: URL(fileURLWithPath: "/usr/bin/env"), arguments: [command] + arguments)
    }

    class func run(logger: ILogger, url: URL, arguments: [String]?) throws -> String {
        let syncHandle = SyncProcessHandle(logger: logger, url: url)
        syncHandle.arguments = arguments

        try syncHandle.run()

        if syncHandle.terminationStatus != 0 {
            let stderr = syncHandle.stderr.contentAsString
            let stdout = syncHandle.stdout.contentAsString
            let out = stderr.isEmpty ? stdout : stderr
            throw Error(commandName: url.lastPathComponent, arguments: arguments, output: out)
        }
        return syncHandle.stdout.contentAsString
    }

    // readyBlock gets called after the Process was asked to start running 
    func run(readyBlock: (() -> Void)? = nil) throws {
        prepareForRun()

        process.terminationHandler = nil

        try super.runUnderlyingProcess()
        readyBlock?()
        waitUntilExit()

        if process.isRunning {
            // We've seen process.isRunning to return true on Linux, even after process.waitUntilExit() has completed
            //
            // We try to work around this with sleep.
            var i = 0
            while process.isRunning {
                if i > 9 {
                    throw CompilerError("Called syncHandle.waitUntilExit() multiple times with sleeps in-between, but process \(process.processIdentifier) still reports that it's running")
                }
                usleep(100000)
                waitUntilExit()
                i += 1
            }
        }
    }

    private func waitUntilExit() {
        // Ensure the pipes are fully read before waiting. This works because the dispatch queue is nil.
        stdout.readToEnd()
        stderr.readToEnd()
        process.waitUntilExit()
    }

    class func usingEnv(logger: ILogger, command: [String]) -> SyncProcessHandle {
        let processHandle = SyncProcessHandle(logger: logger, url: URL(fileURLWithPath: "/usr/bin/env"))
        processHandle.arguments = command
        return processHandle
    }
}
