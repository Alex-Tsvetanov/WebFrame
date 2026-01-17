//
//  USBMuxPacket.swift
//  Compiler
//
//  Created by saniul on 09/10/2019.
//

import Foundation

enum USBMuxPacketProtocol: UInt32 {
    case binary = 0
    case plist = 1
}

enum USBMuxPacketType: UInt32 {
    case result = 1
    case connect = 2
    case listen = 3
    case add = 4
    case remove = 5
    // ??? = 6
    // ??? = 7
    case plistPayload = 8

    var stringValue: String {
        return "\(self)"
    }
}

typealias USBMuxPacketSize = UInt32
typealias USBMuxPacketTag = UInt32
typealias USBMuxPlistPayload = [String: Any]

struct USBMuxPacket {
    let `protocol`: USBMuxPacketProtocol
    let type: USBMuxPacketType
    let tag: USBMuxPacketTag
    let payload: Data

    static var fullSizeLayoutSize: UInt32 {
        return UInt32(MemoryLayout<USBMuxPacketSize>.stride)
    }

    static var metaLayoutSize: UInt32 {
        let metaSize: Int =
            MemoryLayout<USBMuxPacketProtocol.RawValue>.stride +
                MemoryLayout<USBMuxPacketType.RawValue>.stride +
                MemoryLayout<USBMuxPacketTag>.stride
        return UInt32(metaSize)
    }

    var fullPacketSize: USBMuxPacketSize {
        return Self.fullSizeLayoutSize + Self.metaLayoutSize + UInt32(payload.count)
    }

    var fullPacketData: Data {
        var data = Data()
        // packet size
        data.append(integer: fullPacketSize)

        // meta
        data.append(integer: `protocol`.rawValue)
        data.append(integer: type.rawValue)
        data.append(integer: tag)

        // payload
        data.append(payload)
        return data
    }

    init(protocol: USBMuxPacketProtocol, type: USBMuxPacketType, tag: USBMuxPacketTag, payload: Data) {
        self.protocol = `protocol`
        self.type = type
        self.tag = tag
        self.payload = payload
    }

    init?(metaAndPayloadData: Data) {
        // NOTE: data does not include packet size by this point
        let protoRange = (0..<MemoryLayout<USBMuxPacketProtocol.RawValue>.stride)
        let protoRaw: USBMuxPacketProtocol.RawValue = metaAndPayloadData[protoRange]
            .withUnsafePointer { pointer in return pointer.pointee }

        let typeRange = (protoRange.endIndex..<protoRange.endIndex + MemoryLayout<USBMuxPacketType.RawValue>.stride)
        let typeRaw: USBMuxPacketType.RawValue = metaAndPayloadData[typeRange]
            .withUnsafePointer { pointer in return pointer.pointee }

        let tagRange = (typeRange.endIndex..<typeRange.endIndex + MemoryLayout<USBMuxPacketTag>.stride)
        let tag: USBMuxPacketTag = metaAndPayloadData[tagRange]
            .withUnsafePointer { pointer in return pointer.pointee }

        let payload = metaAndPayloadData[tagRange.endIndex...]

        guard let `protocol` = USBMuxPacketProtocol(rawValue: protoRaw),
            let type = USBMuxPacketType(rawValue: typeRaw) else {
                return nil
        }

        self = USBMuxPacket(protocol: `protocol`, type: type, tag: tag, payload: payload)
    }

    private static let bundleName = Bundle.main.infoDictionary?["CFBundleName"]
    private static let bundleVersion = Bundle.main.infoDictionary?["CFBundleVersion"] ?? "1"

    static func packetPayloadWithRequestType(_ requestType: USBMuxRequestType, contents: USBMuxPlistPayload) -> USBMuxPlistPayload {
        let payload: USBMuxPlistPayload

        if let bundleName = bundleName {
            payload = [
                "MessageType": requestType.rawValue,
                "ProgName": bundleName,
                "ClientVersionString": bundleVersion
            ]
        } else {
            payload = [
                "MessageType": requestType.rawValue
            ]
        }

        let finalPayload = payload.merging(contents, uniquingKeysWith: { $1 })
        return finalPayload as USBMuxPlistPayload
    }
}
