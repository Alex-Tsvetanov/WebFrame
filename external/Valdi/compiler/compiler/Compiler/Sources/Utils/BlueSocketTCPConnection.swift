//
//  BlueSocketTCPConnection.swift
//  Compiler
//
//  Created by saniul on 07/10/2019.
//

import Foundation
import BlueSocket

class BlueSocketTCPConnection: TCPConnection {
    weak var delegate: TCPConnectionDelegate?
    var dataProcessor: TCPConnectionDataProcessor?

    let key: String
    let socket: Socket

    private let logger: ILogger
    private let sendQueue: DispatchQueue
    private let recvQueue: DispatchQueue
    private var startedListening: Bool = false
    private var buffer = Data()

    init(logger: ILogger, socket: Socket, queueNamePrefix: String) {
        self.logger = logger
        key = "tcp:\(socket.remoteHostname):\(socket.remotePort)"
        sendQueue = DispatchQueue(label: "\(queueNamePrefix).\(key).send")
        recvQueue = DispatchQueue(label: "\(queueNamePrefix).\(key).recv")
        self.socket = socket
    }

    deinit {
        socket.close()
    }

    func startListening() {
        guard !startedListening else {
            return
        }

        startedListening = true
        recvQueue.async {
            self.receiveLoop()
        }
    }

    func close() {
        closeConnection(error: nil)
    }

    func send(data: Data) {
        sendQueue.async {
            do {
                self.logger.verbose("Writing \(data.count) bytes to socket \(self.key)")
                try self.socket.write(from: data)
            } catch let error {
                self.logger.error("Error while writing to {\(self.key)}: \(error.legibleLocalizedDescription)")
                self.closeConnection(error: error)
            }
        }
    }

    private func receiveLoop() {
        repeat {
            do {
                let bytesRead = try socket.read(into: &buffer)
                if bytesRead == 0 && socket.remoteConnectionClosed {
                    closeConnection(error: nil)
                    return
                } else if bytesRead > 0 {
                    _ = processBuffer(&buffer)
                }
            } catch let error {
                logger.debug("Error while reading from {\(key)}: \(error.legibleLocalizedDescription)")
                closeConnection(error: error)
            }
        } while socket.isActive
    }

    private func closeConnection(error: Error?) {
        logger.debug("Closing reloader connection \(key) with error \(error?.legibleLocalizedDescription ?? "")")
        socket.close()
        delegate?.connection(self, didDisconnectWithError: error)
    }
}
