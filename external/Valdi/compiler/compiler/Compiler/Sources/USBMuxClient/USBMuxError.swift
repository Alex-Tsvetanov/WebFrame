//
//  USBMuxError.swift
//  Compiler
//
//  Created by saniul on 09/10/2019.
//

import Foundation

enum USBMuxError: Error {
    case unknown

    case failedResponse(USBMuxResponseCode)
    case unknownResponse(responseCode: UInt32)

    case ioFailure(NSError)
    case readEmptyData

    case couldntParsePacket
    case unexpectedPacketProtocol
    case unexpectedPacketType
    case invalidPlistPayload
    case timeout 
}
