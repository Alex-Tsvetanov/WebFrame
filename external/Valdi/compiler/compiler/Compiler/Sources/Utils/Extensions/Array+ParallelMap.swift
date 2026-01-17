//
//  Array+ParrallelMap.swift
//  Compiler
//
//  Created by Simon Corsin on 9/19/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

import Foundation

// Allows to quickly enable/disable concurrent processing
// This can help to make compilation consistent and make
// debugging easier.
struct Concurrency {
    static var enableConcurrency = true
}

extension Array {

    func parallelProcess(_ process: (Element) throws -> Void) rethrows {
        _ = try parallelMap(process)
    }

    private func doProcess<T>(index: Int, lock: DispatchSemaphore, outArray: UnsafeMutableBufferPointer<Result<T, Error>>, process: (Element) throws -> T) {
        safeAutorelease {
            let item = self[index]
            do {
                let outItem = try process(item)

                lock.lock {
                    outArray[index] = .success(outItem)
                }
            } catch let caughtError {
                lock.lock {
                    outArray[index] = .failure(caughtError)
                }
            }
        }
    }

    func parallelMap<T>(_ process: (Element) throws -> T) rethrows -> [T] {
        if isEmpty {
            return []
        }

        var outArray = [Result<T, Error>]()
        outArray.reserveCapacity(count)
        for _ in 0..<count {
            outArray.append(.failure(CompilerError("Item not processed")))
        }

        let lock = DispatchSemaphore(value: 1)

        outArray.withUnsafeMutableBufferPointer { bufferPointer in
            if Concurrency.enableConcurrency {
                DispatchQueue.concurrentPerform(iterations: bufferPointer.count) { (index) in
                    self.doProcess(index: index, lock: lock, outArray: bufferPointer, process: process)
                }
            } else {
                for index in 0..<count {
                    doProcess(index: index, lock: lock, outArray: bufferPointer, process: process)
                }
            }
        }

        return try outArray.map { try $0.get() }
    }
}
