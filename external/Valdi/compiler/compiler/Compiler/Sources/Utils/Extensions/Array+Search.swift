//
//  Array+Search.swift
//  Compiler
//
//  Created by Simon Corsin on 12/5/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

import Foundation

public extension Array {

    /**
     Index of using a binary search. The array MUST already be sorted.
     If the sorting is ascending, you should return orderedDescending in the
     comparator if the given element is before your researched element,
     orderedAscending should be returned if the element is after your
     researched element, orderedSame if the element has been found.
     If the sorting is descending, just reverse the logic.
     */
    func index(where comparator: (Element) -> ComparisonResult) -> Index? {
        var lowerBound = 0
        var upperBound = count
        while lowerBound < upperBound {
            let midIndex = lowerBound + (upperBound - lowerBound) / 2
            let result = comparator(self[midIndex])

            switch result {
            case .orderedSame:
                return midIndex
            case .orderedDescending:
                lowerBound = midIndex + 1
            case .orderedAscending:
                upperBound = midIndex
            }
        }
        return nil
    }

}

public extension Array where Element: Equatable {

    func contains(_ other: Element) -> Bool {
        return firstIndex(where: { $0 == other }) != nil
    }

}
