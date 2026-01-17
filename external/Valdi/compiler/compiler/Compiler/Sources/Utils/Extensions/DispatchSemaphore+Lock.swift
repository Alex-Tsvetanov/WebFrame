//
//  DispatchSemaphore+Lock.swift
//  Compiler
//
//  Created by Simon Corsin on 7/26/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

import Foundation

struct LockConfig {
    // Promises will end in failed state if they fail to resolve after 10 minutes.
    static var waitTimeoutSeconds: TimeInterval = 10 * 60
}

extension DispatchSemaphore {

    private func waitOrCrashAfterTimeout() {
        let result = self.wait(timeout: DispatchTime.now() + LockConfig.waitTimeoutSeconds)
        if result == .timedOut {
            KillOnTimeout.notifyTimeout()
        }
    }

    func lock(_ block: () throws -> Void) rethrows {
        self.waitOrCrashAfterTimeout()
        defer {
            self.signal()
        }
        try block()
    }

    func lock<T>(_ block: () throws -> T) rethrows -> T {
        self.waitOrCrashAfterTimeout()
        defer {
            self.signal()
        }
        return try block()
    }

    class func newLock() -> DispatchSemaphore {
        return DispatchSemaphore(value: 1)
    }

}
