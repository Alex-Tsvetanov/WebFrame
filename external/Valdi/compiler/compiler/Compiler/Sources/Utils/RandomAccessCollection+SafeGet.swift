//
//  File.swift
//  
//
//  Created by saniul on 10/10/2019.
//

import Foundation

public extension RandomAccessCollection {
    subscript(safe idx: Index) -> Element? {
        get {
            if indices.contains(idx) {
                return self[idx]
            } else {
                return nil
            }
        }
    }
}
