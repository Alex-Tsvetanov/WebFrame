//
//  Message+orderedSerializedData.swift
//
//  Created by Vasily Fomin on 01/17/24.
//  Copyright © 2024 Snap Inc. All rights reserved.
//
import Foundation
import SwiftProtobuf

public extension Message {

    // A version of serializedData with deterministic order.
    func orderedSerializedData() throws -> Data {
        var options = BinaryEncodingOptions()
        options.useDeterministicOrdering = true
        return try serializedData(options: options)
    }
}
