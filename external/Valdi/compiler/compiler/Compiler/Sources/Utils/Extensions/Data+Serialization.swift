//
//  Data+Serialization.swift
//  Compiler
//
//  Created by Simon Corsin on 3/11/19.
//

import Foundation

extension Data {

    mutating func append(integer: UInt32) {
        var value = integer
        Swift.withUnsafePointer(to: &value) {
            append(UnsafeBufferPointer(start: $0, count: 1))
        }
    }

    private static func alignUp(size: UInt32, alignment: UInt32) -> UInt32 {
        // See details here:
        // https://github.com/KabukiStarship/KabukiToolkit/wiki/Fastest-Method-to-Align-Pointers#observations
        return (size + alignment - 1) & ~(alignment - 1)
    }

    static func computePadding(size: UInt32) -> UInt32 {
        return alignUp(size: size, alignment: 4) - size
    }

    static let valdiLengthFieldPaddingBit: UInt32 = 0x80000000

    mutating func append(valdiData: Data, padding: Bool) {
        var length = UInt32(valdiData.count)
        let dataPadding = padding ? Data.computePadding(size: length) : 0

        if dataPadding > 0 {
            length |= Data.valdiLengthFieldPaddingBit
        }

        append(integer: length)
        append(valdiData)

        for _ in 0..<dataPadding {
            append(0)
        }
    }
}
