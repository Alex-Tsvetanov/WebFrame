//
//  DaemonTCPConnectionAcceptor.swift
//  Compiler
//
//  Created by saniul on 03/02/2019.
//  Copyright © 2019 Snap Inc. All rights reserved.
//

import Foundation
import BlueSocket

final class DaemonTCPConnectionAcceptor: DaemonServiceAutoConnector {
    weak var delegate: DaemonServiceAutoConnectorDelegate?
    let listeningPort: Int

    private let logger: ILogger
    private let listenSocket: Socket
    private let acceptQueue = DispatchQueue(label: "com.snap.valdi.DaemonTCPConnectionAcceptor.accept")
    private var startedAccepting: Bool = false

    init(logger: ILogger, port: Int = 0) throws {
        self.logger = logger
        let socket = try Socket.create(family: .inet, type: .stream, proto: .tcp)
        self.listenSocket = socket
        try socket.listen(on: port)
        self.listeningPort = Int(socket.listeningPort)
        logger.info("Reloader listening on port: \(socket.listeningPort)")
    }

    deinit {
        self.listenSocket.close()
    }

    func startAccepting() {
        guard !startedAccepting else {
            return
        }

        startedAccepting = true
        runAcceptLoop()
    }

    private func runAcceptLoop() {
        acceptQueue.async {
            repeat {
                do {
                    let newSocket = try self.listenSocket.acceptClientConnection()

                    self.logger.info("Accepted reloader connection from {\(newSocket.remoteHostname):\(newSocket.remotePort)}")

                    self.addNewConnection(socket: newSocket)
                } catch {
                    guard let socketError = error as? Socket.Error else {
                        self.logger.error("Unexpected error...")
                        return
                    }

                    self.logger.error("Error while accepting: \(socketError.description)")
                }

            } while self.listenSocket.isActive == true
        }
    }

    private func addNewConnection(socket: Socket) {
        let connection = DaemonTCPConnection(logger: logger, socket: socket)
        delegate?.autoConnector(self, newConnectionEstablished: connection, deviceId: "localhost")
    }

    func createTunnel(logger: ILogger, fromLocalPort localPort: Int, toDeviceId: String, devicePort: Int) throws -> TCPTunnel? {
        return try LocalTCPTunnel(logger: logger, listenPort: localPort, destinationPort: devicePort)
    }
}
