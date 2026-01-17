//
//  String+Regex.swift
//  Compiler
//
//  Created by Brandon Francis on 8/7/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

import Foundation

extension String {

    func matches(regex: NSRegularExpression) -> Bool {
        return getMatch(regex: regex) != nil
    }

    func getMatch(regex: NSRegularExpression) -> NSTextCheckingResult? {
        return getMatch(regex: regex, range: NSRange(location: 0, length: self.utf16.count))
    }

    func getMatch(regex: NSRegularExpression, range: NSRange) -> NSTextCheckingResult? {
        let matches = regex.matches(in: self, options: [], range: range)

        return matches.first(where: { $0.range == range })
    }

    func matches(anyRegex regexes: [NSRegularExpression]) -> Bool {
        let range = NSRange(location: 0, length: self.utf16.count)

        return regexes.contains(where: { self.getMatch(regex: $0, range: range) != nil })
    }

}
