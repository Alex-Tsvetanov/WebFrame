//
//  Data+UnsafePointers.swift
//  Compiler
//
//  Created by saniul on 09/09/2019.
//

import Foundation

extension Data {

    // Shorthand for using `withUnsafeBytes` and accessing the `baseAddress` pointer of the buffer safely
    @inlinable public func withUnsafePointer<ResultType, IntegerBufferType: ExpressibleByIntegerLiteral>(_ body: (UnsafePointer<IntegerBufferType>) throws -> ResultType) rethrows -> ResultType {
        return try withUnsafeBytes { (rawBufferPointer: UnsafeRawBufferPointer) -> ResultType in
            let unsafeBufferPointer = rawBufferPointer.bindMemory(to: IntegerBufferType.self)
            guard let unsafePointer = unsafeBufferPointer.baseAddress else {
                var int: IntegerBufferType = 0
                return try body(&int)
            }
            return try body(unsafePointer)
        }
    }

    // Shorthand for using `withUnsafeMutableBytes` and accessing the `baseAddress` pointer of the buffer safely
    @inlinable public mutating func withUnsafeMutablePointer<ResultType, IntegerBufferType: ExpressibleByIntegerLiteral>(_ body: (UnsafeMutablePointer<IntegerBufferType>) throws -> ResultType) rethrows -> ResultType {
        return try withUnsafeMutableBytes { (rawBufferPointer: UnsafeMutableRawBufferPointer) -> ResultType in
            let unsafeBufferPointer = rawBufferPointer.bindMemory(to: IntegerBufferType.self)
            guard let unsafePointer = unsafeBufferPointer.baseAddress else {
                var int: IntegerBufferType = 0
                return try body(&int)
            }
            return try body(unsafePointer)
        }
    }

    public func fastFirstIndex(of value: UInt8) -> Int? {
        // For some reason firstIndex(of: ) ends up being quite slow
        // This hand-written implementation is 6 times faster in my testing
        return withUnsafeBytes { buffer in
            let count = self.count
            let start = buffer.bindMemory(to: UInt8.self).baseAddress!
            let end = start.advanced(by: count)
            var current = start

            while current < end {
                if current.pointee == value {
                    return start.distance(to: current)
                }

                current = current.advanced(by: 1)
            }

            return nil
        }
    }

}
