//
//  LinesIndexer.swift
//  Compiler
//
//  Created by Simon Corsin on 12/5/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

import Foundation

struct LineOffset {
    let lineOffset: Int
    let characterOffset: Int
}

struct LinesIndexer {

    private var ranges = [NSRange]()
    private let str: NSString

    init(str: String) {
        self.init(str: str as NSString)
    }

    init(str: NSString) {
        var currentRange = NSRange(location: 0, length: str.length)
        let newLines = CharacterSet.newlines

        while true {
            let foundRange = str.rangeOfCharacter(from: newLines, options: [], range: currentRange)
            if foundRange.lowerBound == NSNotFound {
                break
            }

            ranges.append(NSRange(location: currentRange.lowerBound, length: foundRange.upperBound - currentRange.lowerBound))
            currentRange = NSRange(location: foundRange.upperBound, length: str.length - foundRange.upperBound)
        }

        if currentRange.length > 0 {
            ranges.append(currentRange)
        }
        self.str = str
    }

    func substring(start: Int, end: Int) -> String {
        return str.substring(with: NSRange(location: start, length: end - start))
    }

    func characterIndex(forLineIndex lineIndex: Int, column: Int) -> Int {
        return ranges[lineIndex].lowerBound + column
    }

    func lineIndex(atCharacterIndex index: Int) -> LineOffset {
        let rangeIndex = ranges.index { (range) -> ComparisonResult in
            if range.lowerBound > index {
                return .orderedAscending
            } else if range.upperBound < index {
                return .orderedDescending
            }
            return .orderedSame
        }!

        let range = ranges[rangeIndex]

        return LineOffset(lineOffset: rangeIndex, characterOffset: index - range.lowerBound)
    }

}
