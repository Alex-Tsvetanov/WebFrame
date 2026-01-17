//
//  Promise.swift
//  Compiler
//
//  Created by Simon Corsin on 7/26/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

import Foundation
import CoreFoundation

protocol Rejectable {
    func reject(error: Error)
}

class TimeoutError: LocalizedError, CustomStringConvertible {
    let name: String

    init(_ name: String) {
        self.name = name
    }

    public var errorDescription: String? {
        return name
    }

    var description: String {
        return "<TimeoutError: \(name)>"
    }
}

class Promise<T>: Rejectable {

    var result: Result<T, Error>? {
        return semaphore.lock {
            return innerResult
        }
    }

    private var innerResult: Result<T, Error>?

    private let semaphore = DispatchSemaphore(value: 1)
    private let condition = DispatchSemaphore(value: 0)

    private var callbacks = [(Result<T, Error>) -> Void]()

    init() {

    }

    init(result: Result<T, Error>) {
        fulfill(result: result)
    }

    init(data: T) {
        resolve(data: data)
    }

    init(error: Error) {
        reject(error: error)
    }

    func fulfill(result: Result<T, Error>) {
        var callbacks = [(Result<T, Error>) -> Void]()
        semaphore.lock {
            guard self.innerResult == nil else {
                fatalError("Promise is already resolved")
            }
            self.innerResult = result
            callbacks = self.callbacks
            self.callbacks = []
            condition.signal()
        }
        for callback in callbacks {
            callback(result)
        }
    }

    func resolve(data: T) {
        fulfill(result: .success(data))
    }

    func reject(error: Error) {
        fulfill(result: .failure(error))
    }

    @discardableResult func then<T2>(_ completion: @escaping (T) -> T2) -> Promise<T2> {
        return then { (result) -> Promise<T2> in
            let newResult = completion(result)
            return Promise<T2>(data: newResult)
        }
    }

    @discardableResult func then<T2>(_ completion: @escaping (T) throws -> T2) -> Promise<T2> {
        return then { (result) -> Promise<T2> in
            do {
                let newResult = try completion(result)
                return Promise<T2>(data: newResult)
            } catch {
                return Promise<T2>(error: error)
            }
        }
    }

    @discardableResult func then<T2>(_ completion: @escaping (T) -> Promise<T2>) -> Promise<T2> {
        let newPromise = Promise<T2>()

        onComplete { (result) in
            switch result {
            case .success(let data):
                let returnedPromise = completion(data)
                returnedPromise.onComplete { (result) in
                    newPromise.fulfill(result: result)
                }
            case .failure(let error):
                newPromise.reject(error: error)
            }
        }

        return newPromise
    }

    func dispatch(on queue: DispatchQueue) -> Promise<T> {
        let newPromise = Promise<T>()

        onComplete { (result) in
            queue.async {
                newPromise.fulfill(result: result)
            }
        }

        return newPromise
    }

    func `catch`(_ completion: @escaping (Error) -> Void) -> Promise<T> {
        return self.catch { (error) -> Error in
            completion(error)
            return error
        }
    }

    func `catch`(_ completion: @escaping (Error) -> Error) -> Promise<T> {
        return self.catch { (error) -> Promise<T> in
            let newError = completion(error)
            return Promise(error: newError)
        }
    }

    func `catch`(_ completion: @escaping (Error) -> Promise<T>) -> Promise<T> {
        let newPromise = Promise<T>()

        onComplete { (result) in
            switch result {
            case .success(let data):
                newPromise.resolve(data: data)
            case .failure(let error):
                let returnedPromise = completion(error)
                returnedPromise.onComplete { (result) in
                    newPromise.fulfill(result: result)
                }
            }
        }

        return newPromise
    }

    func onComplete(_ completion: @escaping (Result<T, Error>) -> Void) {
        var result: Result<T, Error>?
        semaphore.lock {
            if self.innerResult == nil {
                callbacks.append(completion)
            } else {
                result = innerResult
            }
        }

        if let result {
            completion(result)
        }
    }

    private func waitForResultDidTimeOut(timeoutSeconds: TimeInterval) -> Result<T, Error> {
        return .failure(TimeoutError("Promise failed to resolve in \(timeoutSeconds) seconds"))
    }

    func waitForResult(maxWaitTimeSeconds: TimeInterval = LockConfig.waitTimeoutSeconds) -> Result<T, Error> {
        if Thread.isMainThread {
            let expirationTimeSeconds = CFAbsoluteTimeGetCurrent() + maxWaitTimeSeconds
            var result = self.result
            while result == nil {
                if CFAbsoluteTimeGetCurrent() > expirationTimeSeconds {
                    return waitForResultDidTimeOut(timeoutSeconds: maxWaitTimeSeconds)
                }

                // To avoid locking the main thread, we just run it until
                // we get a result. This is because some completion logic may
                // rely on enqueing into the main thread to complete
                let _ = RunLoop.main.run(mode: .default, before: Date().addingTimeInterval(0.05))
                result = self.result
            }
            return result!
        } else {
            switch condition.wait(timeout: DispatchTime.now() + maxWaitTimeSeconds) {
            case .timedOut:
                return waitForResultDidTimeOut(timeoutSeconds: maxWaitTimeSeconds)
            case .success:
                condition.signal()
                return result!
            }
        }
    }

    func waitForData(maxWaitTimeSeconds: TimeInterval = LockConfig.waitTimeoutSeconds) throws -> T {
        let result = waitForResult(maxWaitTimeSeconds: maxWaitTimeSeconds)
        switch result {
        case .success(let data):
            return data
        case .failure(let error):
            throw error
        }
    }

    class func of<C: Collection>(promises: C) -> Promise<[T]> where C.Element == Promise<T> {
        if promises.isEmpty {
            return Promise<[T]>(data: [])
        }

        let outPromise = Promise<[T]>()
        let syncResults = Synchronized(data: [Result<T, Error>?](repeating: nil, count: promises.count))
        var completedPromises = 0

        for (index, promise) in promises.enumerated() {
            promise.onComplete { (result) in
                syncResults.data { (results) in
                    results[index] = result
                    completedPromises += 1

                    if completedPromises == results.count {
                        var allResults = [T]()
                        allResults.reserveCapacity(results.count)
                        for existingResult in results {
                            do {
                                let data = try existingResult!.get()
                                allResults.append(data)
                            } catch let error {
                                outPromise.reject(error: error)
                                return
                            }
                        }

                        outPromise.resolve(data: allResults)
                    }
                }

            }
        }

        return outPromise
    }

}
