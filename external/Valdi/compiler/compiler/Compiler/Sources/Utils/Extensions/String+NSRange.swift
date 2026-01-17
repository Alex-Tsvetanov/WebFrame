//
//  String+NSRange.swift
//  Compiler
//
//  Created by Nathaniel Parrott on 7/17/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

import Foundation

// From https://nshipster.com/nsregularexpression/

extension String {

    /// An `NSRange` that represents the full range of the string.
    var nsrange: NSRange {
        return NSRange(location: 0, length: utf16.count)
    }

    /// Returns a substring with the given `NSRange`,
    /// or `nil` if the range can't be converted.
    func substring(with nsrange: NSRange) -> String? {
        guard let range = Range(nsrange)
            else { return nil }

        let start = String.Index(utf16Offset: range.lowerBound, in: self)
        let end = String.Index(utf16Offset: range.upperBound, in: self)
        return String(utf16[start..<end])
    }

    func replacing(range nsrange: NSRange, with str: String) -> String? {
        guard let range = Range(nsrange)
            else { return nil }

        let start = String.Index(utf16Offset: range.lowerBound, in: self)
        let end = String.Index(utf16Offset: range.upperBound, in: self)

        return (String(utf16[utf16.startIndex..<start]) ?? "") + str + (String(utf16[end..<utf16.endIndex]) ?? "")
    }

    /// Returns a range equivalent to the given `NSRange`,
    /// or `nil` if the range can't be converted.
    func range(from nsrange: NSRange) -> Range<Index>? {
        guard let range = Range(nsrange) else { return nil }
        let utf16Start = String.Index(utf16Offset: range.lowerBound, in: self)
        let utf16End = String.Index(utf16Offset: range.upperBound, in: self)

        guard let start = Index(utf16Start, within: self),
            let end = Index(utf16End, within: self)
            else { return nil }

        return start..<end
    }
}
