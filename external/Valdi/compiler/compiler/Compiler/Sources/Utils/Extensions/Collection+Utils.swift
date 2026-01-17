//
//  Collection+Utils.swift
//  Compiler
//
//  Created by David Byttow on 10/19/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

import Foundation

public extension Sequence {
    func count(_ closure: (Element) -> Bool) -> Int {
        return reduce(into: 0) { (acc: inout Int, item) in
            if closure(item) {
                acc += 1
            }
        }
    }

    func associate<Key: Hashable, Value>(_ closure: (Element) -> (Key, Value)) -> [Key: Value] {
        var out = [Key: Value]()

        for element in self {
            let (key, value) = closure(element)
            out[key] = value
        }

        return out
    }

    /**
     Similar to Dictionary(uniqueKeysWithValues:), but will throw on duplicate key instead of asserting and crashing the
     program unsafely.
     */
    func associateUnique<Key: Hashable, Value>(_ closure: (Element) -> (Key, Value)) throws -> [Key: Value] {
        var out = [Key: Value]()

        for element in self {
            let (key, value) = closure(element)
            guard out[key] == nil else {
                throw CompilerError("Conflicting key '\(key)'")
            }
            out[key] = value
        }

        return out
    }

    func associateBy<Key: Hashable>(_ closure: (Element) -> Key) -> [Key: Element] {
        return associate { (element) -> (Key, Element) in
            return (closure(element), element)
        }
    }

    func group<T: Hashable, Value>(_ closure: (Element) -> (T, Value)) -> [T: [Value]] {
        var out = [T: [Value]]()

        for element in self {
            let (key, value) = closure(element)
            out[key, default: []].append(value)
        }

        return out
    }

    func groupBy<T: Hashable>(_ closure: (Element) -> T) -> [T: [Element]] {
        var out = [T: [Element]]()

        for element in self {
            let key = closure(element)
            out[key, default: []].append(element)
        }

        return out
    }

}

extension Array where Element: Comparable {
    func replacingPrefix(_ prefix: [Element], replacement: [Element]) -> [Element] {
        guard self.starts(with: prefix) else {
            return self
        }

        return replacement + Array(dropFirst(prefix.count))
    }
}

public extension MutableCollection {

    mutating func transform(_ closure: (inout Element) -> Void) {
        var currentIndex = startIndex
        let endIndex = self.endIndex
        while currentIndex != endIndex {
            closure(&self[currentIndex])

            currentIndex = self.index(currentIndex, offsetBy: 1)
        }
    }

}

extension Collection {
    var nonEmpty: Self? {
        return isEmpty ? nil : self
    }
}
