//
//  File.swift
//  
//
//  Created by saniul on 15/11/2019.
//

import Foundation
import Crypto

extension Data {
    func generateSHA256Digest() throws -> String {
        let digest = Crypto.SHA256.hash(data: self)
        var out = ""
        out.reserveCapacity(64)
        for element in digest {
            out += String(format: "%02x", element)
        }
        return out
    }
}
