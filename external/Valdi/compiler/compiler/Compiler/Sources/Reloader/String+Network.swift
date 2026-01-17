//
//  Utils.swift
//  Compiler
//
//  Created by saniul on 04/02/2019.
//  Copyright © 2019 Snap Inc. All rights reserved.
//

import Foundation

extension String {
    func toIpAddress() throws -> UInt32 {
        let components = self.components(separatedBy: ".").compactMap { UInt32($0) }
        guard components.count == 4 else {
            throw CompilerError("Invalid ip address: " + self)
        }

        var out: UInt32 = 0
        for (index, component) in components.enumerated() {
            guard component < 255 else {
                throw CompilerError("Invalid ip address: " + self)
            }
            out |= component << (8 * (3 - index))
        }
        return out.bigEndian
    }

}
