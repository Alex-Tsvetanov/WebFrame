//
//  DaemonTCPConnection.swift
//  Compiler
//
//  Created by Simon Corsin on 2/15/19.
//  Copyright © 2019 Snap Inc. All rights reserved.
//

import Foundation
import BlueSocket

protocol DaemonTCPConnectionDelegate: AnyObject {
    func connection(_ connection: DaemonTCPConnection, didReceiveData data: Data)
    func connection(_ connection: DaemonTCPConnection, didDisconnectWithError error: Error?)
}

private struct ValdiProtocol: TCPConnectionDataProcessor {
    func process(data: Data) -> TCPConnectionProcessDataResult {
        if data.count < 8 {
            return .notEnoughData(lengthNeeded: 8 - data.count)
        }

        if !data.starts(with: Magic.valdiMagic) {
            return .invalid(dataToDiscard: 1)
        }

        let dataLength: UInt32 = data.withUnsafePointer { bufferStart in
            return bufferStart.advanced(by: 1).pointee
        }

        if dataLength > data.count - 8 {
            // Not enough data yet
            return .notEnoughData(lengthNeeded: Int(dataLength) - (data.count - 8))
        }

        return .valid(dataLength: Int(dataLength) + 8)
    }
}

final class DaemonTCPConnection: TCPConnectionDelegate {
    weak var delegate: DaemonTCPConnectionDelegate?

    var key: String {
        return connection.key
    }

    var everReadPacket: Bool = false

    private let connection: TCPConnection
    private var disconnectHandlers: [((Error?) -> Void)] = []

    private var additionalDisconnectHandlers: [((Error?) -> Void)] = []

    convenience init(logger: ILogger, socket: Socket) {
        let connection = BlueSocketTCPConnection(logger: logger, socket: socket, queueNamePrefix: "DaemonTCPConnection")
        self.init(connection: connection)
    }

    convenience init(logger: ILogger, key: String, channel: DispatchIO) {
        let connection = DispatchChannelTCPConnection(logger: logger, key: key, ioChannel: channel, queueNamePrefix: "DaemonTCPConnection")
        self.init(connection: connection)
    }

    init(connection: TCPConnection) {
        self.connection = connection
        self.connection.dataProcessor = ValdiProtocol()
        self.connection.delegate = self
    }

    func startListening() {
        connection.startListening()
    }

    func close() {
        connection.close()
    }

    func send(data: Data) {
        var packetData = Data(capacity: 8 + data.count)
        packetData.append(Magic.valdiMagic)

        var dataLength = UInt32(data.count)
        Swift.withUnsafePointer(to: &dataLength) {
            packetData.append(UnsafeBufferPointer(start: $0, count: 1))
        }
        packetData.append(data)

        connection.send(data: packetData)
    }

    func addDisconnectHandler(_ handler: @escaping (Error?) -> Void) {
        self.disconnectHandlers.append(handler)
    }

    func connection(_ connection: TCPConnection, didReceiveData data: Data) {
        let packet = data[8...]
        everReadPacket = true
        delegate?.connection(self, didReceiveData: packet)
    }

    func connection(_ connection: TCPConnection, didDisconnectWithError error: Error?) {
        delegate?.connection(self, didDisconnectWithError: error)
        for handler in disconnectHandlers {
            handler(error)
        }
        disconnectHandlers = []
    }
}

extension DaemonTCPConnection: CustomStringConvertible {

    var description: String {
        return key
    }

}
