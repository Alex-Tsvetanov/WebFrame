//
//  Throttler.swift
//  Compiler
//
//  Created by Simon Corsin on 12/11/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

import Foundation

class AsyncTaskQueue {

    private let maxOperationsCount: Int

    private let lock = DispatchSemaphore.newLock()
    private var pendingOperations = [(@escaping () -> Void) -> Void]()
    private var currentOperationsCount = 0

    init(maxOperationsCount: Int) {
        self.maxOperationsCount = maxOperationsCount
    }

    func enqueue<T>(_ work: @escaping  () -> Promise<T>) -> Promise<T> {
        let promise = Promise<T>()

        onReady { completion in
            _ = work().then { (data) -> Void in
                promise.resolve(data: data)
                completion()
                }.catch { (error) -> Void in
                    promise.reject(error: error)
                    completion()
            }
        }

        return promise
    }

    private func dequeueIfNeeded() -> ((@escaping () -> Void) -> Void)? {
        return lock.lock {
            // should have the lock
            guard currentOperationsCount < maxOperationsCount, !pendingOperations.isEmpty else {
                return nil
            }

            currentOperationsCount += 1
            return pendingOperations.removeFirst()
        }
    }

    private func runOperations() {
        while true {
            guard let work = dequeueIfNeeded() else {
                return
            }

            work {
                self.lock.lock {
                    self.currentOperationsCount -= 1
                }
                self.runOperations()
            }
        }
    }

    private func onReady(_ work: @escaping (@escaping () -> Void) -> Void) {
        lock.lock {
            pendingOperations.append(work)
        }
        runOperations()
    }

}
