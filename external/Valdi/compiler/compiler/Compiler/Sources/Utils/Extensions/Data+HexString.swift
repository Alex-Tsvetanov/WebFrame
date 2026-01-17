//
//  Data+HexString.swift
//  
//
//  Created by saniul on 04/11/2019.
//

import Foundation

extension Data {
    init?(hexString: String) {
        guard hexString.count % 2 == 0 else { return nil }

        var result = Data(capacity: hexString.count/2)

        for i in hexString.utf8.strideIndices(by: 2) {
            let byteRange = i...hexString.index(after: i)
            guard let byte = UInt8(hexString[byteRange], radix: 16) else { return nil }
            result.append(byte)
        }

        self = result
    }

    var hexString: String {
        self.map { String(format: "%02x", $0) }.joined()
    }
}
