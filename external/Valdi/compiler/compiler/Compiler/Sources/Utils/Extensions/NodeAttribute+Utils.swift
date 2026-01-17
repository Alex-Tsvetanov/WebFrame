//
//  NodeAttribute+Utils.swift
//  Compiler
//
//  Created by Simon Corsin on 5/27/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

import Foundation

private let decimalDigits = CharacterSet.decimalDigits

extension Valdi_NodeAttribute {

    init(logger: ILogger, name: String, value: String) {
        self.name = name

        var isString = true

        let trimmed = value.removingCharacters(in: decimalDigits)
        if trimmed.isEmpty || trimmed == "px" || trimmed == "pt" {
            var int: Int64 = 0

            if Scanner(string: value).scanInt64(&int) {
                intValue = int
                type = .nodeAttributeTypeInt
                isString = false
            } else {
                logger.warn("Was not able to convert auto-detected int with value '\(value)' to Int")
            }
        } else if trimmed == "." || trimmed == ".px" || trimmed == ".pt" {
            if let double = Scanner(string: value).scanDouble() {
                let behavior = NSDecimalNumberHandler(roundingMode: .plain,
                                                      scale: 4,
                                                      raiseOnExactness: false,
                                                      raiseOnOverflow: false,
                                                      raiseOnUnderflow: false,
                                                      raiseOnDivideByZero: true)
                // Rounding the parsed Double here because we noticed we get different behavior on macOS and Linux, e.g.
                // ".95" would parse to 0.95 on macOS, but 0.0.950000001 on Linux
                let decimal = NSDecimalNumber(value: double).rounding(accordingToBehavior: behavior)
                doubleValue = decimal.doubleValue
                type = .nodeAttributeTypeDouble
                isString = false
            } else {
                logger.warn("Was not able to convert auto-detected double with value '\(value)' to Double")
            }
        }

        if isString {
            self.strValue = value
            type = .nodeAttributeTypeString
        }
    }

}
