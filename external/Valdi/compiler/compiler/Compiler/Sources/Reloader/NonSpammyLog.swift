//
//  NonSpammyLog.swift
//  Compiler
//
//  Created by saniul on 10/05/2019.
//

import Foundation

class NonSpammyLogger<Key: Hashable> {
    private var countsByKey: Synchronized<[Key: Int]> = Synchronized(data: [:])

    private let logger: ILogger

    init(logger: ILogger) {
        self.logger = logger
    }

    func log(level: LogLevel, maxCount: Int, key: Key, _ message: @autoclosure () -> String, _ functionStr: StaticString = #function) {
        countsByKey.data { countsByKey in
            var currentCount = countsByKey[key] ?? 0

            currentCount += 1
            guard currentCount <= maxCount else {
                return
            }

            logger.log(level: level, message, functionStr: functionStr)

            if currentCount == maxCount {
                logger.log(level: level, { "...won't log further messages like ^that to avoid spamming the logs" }, functionStr: functionStr)
            }

            countsByKey[key] = currentCount
        }
    }

    func reset(key: Key) {
        countsByKey.data { countsByKey in
            countsByKey[key] = 0
        }
    }

    func reset(matching predicate: (Key) -> Bool) {
        countsByKey.data { countsByKey in
            let keysToRemove = countsByKey.filter { key, _ in predicate(key) }.keys
            keysToRemove.forEach { countsByKey[$0] = nil }
        }
    }
}
