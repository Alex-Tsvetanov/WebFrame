//
//  Logger.swift
//  Compiler
//
//  Created by Simon Corsin on 4/6/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

import Foundation
import Chalk

enum LogLevel: Int, CaseIterable, Codable {
    case trace = -2
    case verbose = -1
    case debug = 0
    case info = 1
    case warn = 2
    case error = 3
}

extension LogLevel {

    var description: String {
        switch self {
        case .trace:
            return "TRACE"
        case .verbose:
            return "VERBOSE"
        case .debug:
            return "DEBUG"
        case .info:
            return "INFO"
        case .warn:
            return "WARN"
        case .error:
            return "ERROR"
        }
    }

    var color: Chalk.Color {
        switch self {
        case .trace:
            return .white
        case .verbose:
            return .white
        case .debug:
            return .white
        case .info:
            return .blue
        case .warn:
            return .yellow
        case .error:
            return .red
        }
    }

    var style: Chalk.Style {
        switch self {
        case .trace:
            return .dim
        case .verbose:
            return .dim
        case .debug:
            return .normal
        case .info:
            return .bold
        case .warn:
            return .bold
        case .error:
            return .bold
        }
    }

    init?(description: String) {
        switch description {
        case "TRACE":
            self = .trace
        case "VERBOSE":
            self = .verbose
        case "DEBUG":
            self = .debug
        case "INFO":
            self = .info
        case "WARN":
            self = .warn
        case "ERROR":
            self = .error
        default:
            return nil
        }
    }

}

extension Chalk.Style {
    public static let normal = Style([])
}

protocol LoggerInterceptor: AnyObject {
    func onInterceptLog(message: String, functionStr: StaticString)
}

protocol ILogger: AnyObject {
    var minLevel: LogLevel { get set }
    var duplicateStderrToStdout: Bool { get set }
    var interceptor: LoggerInterceptor? { get set }
    var emittedLogsCount: Int { get }

    func log(level: LogLevel, _ message: () -> String, functionStr: StaticString)
    func flush()
}

protocol ILoggerOutput: AnyObject {
    func write(data: Data, isError: Bool)
}

class FileHandleLoggerOutput: ILoggerOutput {

    private let stdout: FileHandle
    private let stderr: FileHandle

    init(stdout: FileHandle, stderr: FileHandle) {
        self.stdout = stdout
        self.stderr = stderr
    }

    private func doWrite(data: Data, fileHandle: FileHandle) throws {
        if #available(OSX 10.15.4, *) {
            try fileHandle.write(contentsOf: data)
        } else {
            fileHandle.write(data)
        }
    }

    func write(data: Data, isError: Bool) {
        do {
            if isError {
                // Flush stdout, to make sure the errors are displayed in the right order
                try? self.stdout.synchronize()

                try doWrite(data: data, fileHandle: self.stderr)
            } else {
                try doWrite(data: data, fileHandle: self.stdout)
            }
        } catch let error {
            print("[FATAL] FAILED TO WRITE MESSAGE TO stream! Source location: Error: \(error.legibleLocalizedDescription)")
        }
    }

}

class BufferLoggerOutput: ILoggerOutput {    
    var data: Data {
        return lock.lock {
            self.buffer
        }
    }

    private let lock = DispatchSemaphore.newLock()
    private var buffer = Data()

    func write(data: Data, isError: Bool) {
        lock.lock {
            self.buffer.append(data)
        }
    }
}

final class Logger: ILogger {
    var minLevel = LogLevel.info
    var duplicateStderrToStdout = false
    var interceptor: LoggerInterceptor?
    var asyncLogs = true

    var emittedLogsCount: Int {
        return queue.sync { _emittedLogsCount }
    }

    private var _emittedLogsCount = 0
    private let startTime = Date.timeIntervalSinceReferenceDate
    private let queue = DispatchQueue(label: "com.snap.valdi.Logger")
    private let output: ILoggerOutput

    init(output: ILoggerOutput) {
        self.output = output
    }

    func flush() {
        queue.sync {}
    }

    private func makeMessage(level: LogLevel, timeOffset: TimeInterval, _ message: () -> String) -> String {
        let tag = "[\(level.description)]"
        return "[\(String(format: "%.3f", timeOffset))] \(tag, color: level.color, style: level.style) \(message())"
    }

    func log(level: LogLevel, _ message: () -> String, functionStr: StaticString) {
        let timeOffset = Date.timeIntervalSinceReferenceDate - startTime

        guard isEnabled(forLevel: level) else {
            if let interceptor {
                let message =  makeMessage(level: level, timeOffset: timeOffset, message)
                interceptor.onInterceptLog(message: message, functionStr: functionStr)
            }

            return
        }

        let message =  makeMessage(level: level, timeOffset: timeOffset, message)
        if let interceptor {
            interceptor.onInterceptLog(message: message, functionStr: functionStr)
        }

        if asyncLogs {
            queue.async { self.doLog(level: level, message: message, functionStr: functionStr) }
        } else {
            queue.sync { self.doLog(level: level, message: message, functionStr: functionStr) }
        }
    }

    private func doLog(level: LogLevel, message: String, functionStr: StaticString) {
        let messageData = try! "\(message)\n".utf8Data()

        if level.rawValue >= LogLevel.error.rawValue {
            if duplicateStderrToStdout {
                output.write(data: messageData, isError: false)
            }

            output.write(data: messageData, isError: true)
        } else {
            output.write(data: messageData, isError: false)
        }

        _emittedLogsCount += 1
    }

    func isEnabled(forLevel level: LogLevel) -> Bool {
        return minLevel.rawValue <= level.rawValue
    }
}

extension ILogger {

    func trace(_ message: @autoclosure () -> String, functionStr: StaticString = #function) {
        log(level: .trace, message, functionStr: functionStr)
    }

    func verbose(_ message: @autoclosure () -> String, functionStr: StaticString = #function) {
        log(level: .verbose, message, functionStr: functionStr)
    }

    func debug(_ message: @autoclosure () -> String, functionStr: StaticString = #function) {
        log(level: .debug, message, functionStr: functionStr)
    }

    func info(_ message: @autoclosure () -> String, functionStr: StaticString = #function) {
        log(level: .info, message, functionStr: functionStr)
    }

    func warn(_ message: @autoclosure () -> String, functionStr: StaticString = #function) {
        log(level: .warn, message, functionStr: functionStr)
    }

    func error(_ message: @autoclosure () -> String, functionStr: StaticString = #function) {
        log(level: .error, message, functionStr: functionStr)
    }
}
