//
//  BatchSequence.swift
//  Compiler
//
//  Created by saniul on 26/06/2019.
//

import Foundation

struct BatchSequence<T>: Sequence, IteratorProtocol {

    private let array: [T]
    private let distance: Int
    private var index = 0

    init(array: [T], distance: Int) {
        precondition(distance > 0 || (distance == 0 && array.isEmpty), "distance must be greater than 0") // prevents infinite loop
        self.array = array
        self.distance = distance
    }

    mutating func next() -> [T]? {
        guard index < array.endIndex else { return nil }
        let newIndex = index.advanced(by: distance) > array.endIndex ? array.endIndex : index.advanced(by: distance)
        defer { index = newIndex }
        return Array(array[index ..< newIndex])
    }

}
