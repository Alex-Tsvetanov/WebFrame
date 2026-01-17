// Copyright © 2024 Snap, Inc. All rights reserved.

import Foundation
import Collections
import Backtrace

class LoggerInterceptorImpl: LoggerInterceptor {

    private var logs = Deque<Log>()
    private let lock = DispatchSemaphore.newLock()

    private static let maxLogSize = 1000

    struct Log {
        let message: String
    }

    func onInterceptLog(message: String, functionStr: StaticString) {
        lock.lock {
            logs.append(Log(message: message))
            while logs.count > LoggerInterceptorImpl.maxLogSize {
                logs.removeFirst()
            }
        }
    }

    func collectLogs() -> [Log] {
        return lock.lock {
            logs.map { $0 }
        }
    }
}

class KillOnTimeout {

    private static var allInstances = Synchronized<[KillOnTimeout]>(data: [])

    private let logger: ILogger
    private let timeoutSeconds: TimeInterval
    private var task: Cancelable?
    private let queue = DispatchQueue(label: "com.valdi.compiler.KillOnTimeout", qos: .default)
    private let logs = LoggerInterceptorImpl()

    init(logger: ILogger, timeoutSeconds: TimeInterval) {
        self.logger = logger
        self.timeoutSeconds = timeoutSeconds
    }

    func start() {
        logger.interceptor = logs

        let task = CancelableBlock { [weak self] in
            self?.timeoutDetected()
        }
        self.queue.asyncAfter(deadline: .now() + timeoutSeconds, execute: task.run)

        KillOnTimeout.allInstances.data { array in
            array.append(self)
        }
    }

    func stop() {
        task?.cancel()
        task = nil

        KillOnTimeout.allInstances.data { array in
            array.removeAll { instance in
                return instance === self
            }
        }
    }

    private func notifyTimeoutDetectedNow() {
        queue.sync {
            self.timeoutDetected()
        }
    }

    private func logToStdError(message: String) {
        let messageData = try! "\(message)\n".utf8Data()
        let _ = try? FileHandle.standardError.write(contentsOf: messageData)
    }

    private func timeoutDetected() {
        let allLogs = logs.collectLogs()
        logToStdError(message: "\n\n\nSHOWING LAST \(allLogs.count) LOGS...\n\n\n")
        for l in allLogs {
            logToStdError(message: l.message)
        }

        logToStdError(message: "\n\n\nPROCESS DID TIMEOUT AFTER \(timeoutSeconds) seconds!")
        let _ = try? FileHandle.standardError.synchronize()
        raise(SIGQUIT)

        // Force kill process if it didn't kill itself already
        queue.asyncAfter(deadline: .now() + TimeInterval(2)) {
            Backtrace.print()
            fflush(stderr)

            _exit(1)
        }
    }

    static func notifyTimeout() {
        for instance in KillOnTimeout.allInstances.unsafeData {
            instance.timeoutDetected()
        }
    }
}
