//
//  WatchmanConnection.swift
//  BlueSocket
//
//  Created by Simon Corsin on 6/5/19.
//

import Foundation
import BlueSocket

struct WatchmanProtocol: TCPConnectionDataProcessor {

    func process(data: Data) -> TCPConnectionProcessDataResult {
        guard let newLineIndex = data.fastFirstIndex(of: 10) else {
            return .notEnoughData(lengthNeeded: 1)
        }
        return .valid(dataLength: newLineIndex + 1)
    }
}

protocol WatchmanConnectionDelegate: AnyObject {
    func watchmanConnection(_ watchmanConnection: WatchmanConnection, didReceiveUnilateralPacket packet: Data)
}

class WatchmanConnection: TCPConnectionDelegate {

    weak var delegate: WatchmanConnectionDelegate?

    private static let unilateralToken = try! "\"unilateral\":true".utf8Data()
    private static let errorToken = try! "\"error\":".utf8Data()

    private let logger: ILogger
    private let socket: Socket
    private let queue = DispatchQueue(label: "com.snap.valdi.WatchmanClient")
    private let tcpConnection: TCPConnection
    private var sequence = 0
    private var packetObservers = [Promise<Data>]()

    init(logger: ILogger, sockname: String) throws {
        self.logger = logger
        guard let signature = try Socket.Signature(socketType: .stream, proto: .unix, path: sockname) else {
            throw CompilerError("Could not create signature from socket at \(sockname)")
        }
        self.socket = try Socket.create(connectedUsing: signature)

        self.tcpConnection = BlueSocketTCPConnection(logger: logger, socket: socket, queueNamePrefix: "com.snap.valdi.WatchmanClient")
        self.tcpConnection.dataProcessor = WatchmanProtocol()
        self.tcpConnection.delegate = self

        self.tcpConnection.startListening()
    }

    func send(data: Data) -> Promise<Data> {
        let promise = Promise<Data>()
        queue.async {
            self.packetObservers.append(promise)
            var outData = data
            outData.append(10)
            self.tcpConnection.send(data: outData)
        }

        return promise
    }

    func connection(_ connection: TCPConnection, didReceiveData data: Data) {
        queue.async {
            let packet = data[..<(data.count - 1)]
            if packet.range(of: WatchmanConnection.unilateralToken) != nil {
                self.delegate?.watchmanConnection(self, didReceiveUnilateralPacket: packet)
            } else {
                if self.packetObservers.isEmpty {
                    self.logger.error("Received unhandled packet of size \(data.count)")
                    return
                }
                let promise = self.packetObservers.removeFirst()

                if packet.range(of: WatchmanConnection.errorToken) != nil {
                    let errorMessage: String
                    do {
                        let errorPacket = try WatchmanErrorResponse.fromJSON(packet)
                        errorMessage = errorPacket.error
                    } catch let error {
                        errorMessage = "Failed to deserialize error \(error.legibleLocalizedDescription)"
                    }
                    promise.reject(error: CompilerError(errorMessage))
                } else {
                    promise.resolve(data: packet)
                }
            }
        }
    }

    func connection(_ connection: TCPConnection, didDisconnectWithError error: Error?) {

    }

}
