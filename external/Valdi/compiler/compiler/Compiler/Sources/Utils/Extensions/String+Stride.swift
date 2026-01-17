//
//  File.swift
//  
//
//  Created by saniul on 04/11/2019.
//

import Foundation

// Swift's String.UTF8View.Index is not Strideable, see discussion:
// https://stackoverflow.com/questions/36205309/conforming-string-characterview-index-to-strideable-fatal-error-when-using-stri
//
// This extension is loosely based on https://stackoverflow.com/a/36206979
extension String.UTF8View {
    typealias Index = String.UTF8View.Index

    func strideIndices(by stride: Int) -> AnySequence<Index> {
        strideIndices(from: startIndex, to: endIndex, by: stride)
    }

    func strideIndices(from start: Index, to end: Index, by stride: Int) -> AnySequence<Index> {
        precondition(stride != 0, "stride size must not be zero")

        return AnySequence { () -> AnyIterator<Index> in
            var current = start
            return AnyIterator {
                guard let next = self.index(current, offsetBy: stride, limitedBy: end) else {
                    return nil
                }

                let result = current
                current = next
                return result
            }
        }
    }
}
