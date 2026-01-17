//
//  String+Concatenate.swift
//  Compiler
//
//  Created by Ivan Golub on 6/7/22.
//  Copyright © 2022 Snap Inc. All rights reserved.
//

import Foundation

extension String {
    static func concatenate(_ components: String?...,
                               separator: String = "",
                               transform: (String?) -> String? = { $0 }) -> String? {
        return components
            .compactMap { transform($0) }
            .joined(separator: separator)
    }
}
