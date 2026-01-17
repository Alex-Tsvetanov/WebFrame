//
//  TCPTunnel.swift
//  Compiler
//
//  Created by elee2 on 02/22/2024.
//

import Foundation
import BlueSocket

enum TCPTunnelError: Error {
    case unimplemented
}

protocol TCPTunnelManagerDelegate: AnyObject {
    func tunnelConnectionCreated(_ tunnel: TCPTunnel, connection: TCPTunnelConnection, from: TCPConnection, to: TCPConnection)
    func tunnelConnectionDisconnected(_ tunnel: TCPTunnel, connection: TCPTunnelConnection)
    func tunnelConnectionError(_ tunnel: TCPTunnel, error: Error)
}

class TCPTunnel {
    let listeningPort: Int
    weak var delegate: TCPTunnelManagerDelegate?

    let logger: ILogger
    private let listenSocket: Socket
    private var startedAccepting: Bool = false
    private var connections: [TCPTunnelConnection] = []

    private let acceptQueue = DispatchQueue(label: "TCPTunnel.acceptQueue")
    private let queue = DispatchQueue(label: "TCPTunnel")

    init(logger: ILogger, port: Int) throws {
        self.logger = logger
        self.listenSocket = try Socket.create(family: .inet, type: .stream, proto: .tcp)
        try self.listenSocket.listen(on: port)
        self.listeningPort = Int(self.listenSocket.listeningPort)
    }

    func destinationConnectionProvider() throws -> Promise<TCPConnection> {
        throw TCPTunnelError.unimplemented
    }

    func start () {
        guard !startedAccepting else {
            return
        }
        startedAccepting = true
        runAcceptLoop()
    }

    func stop () {
        queue.async { [weak self] in
            guard let self = self else { return }
            self.listenSocket.close()
            for connection in self.connections {
                connection.stop()
            }
            self.connections.removeAll()
        }
    }

    private func runAcceptLoop() {
        acceptQueue.async {
            repeat {
                do {
                    let newSocket = try self.listenSocket.acceptClientConnection()
                    let connectionA = BlueSocketTCPConnection(logger: self.logger, socket: newSocket, queueNamePrefix: "TunnelServer")
                    try self.destinationConnectionProvider().onComplete { [weak self, connectionA] (result) in
                        switch result {
                        case .success(let connectionB):
                            self?.connectTunnel(from: connectionA, to: connectionB)
                        case .failure(let error):
                            self?.logger.error("[TCPTunnel] Error completing tunnel connection: \(error)")
                            connectionA.close()
                            self?.delegate?.tunnelConnectionError(self!, error: error)
                        }
                    }
                } catch {
                    if let socketError = error as? Socket.Error {
                        self.logger.debug("[TCPTunnel] Error while accepting: \(socketError.description)")
                    } else {
                        self.logger.error("[TCPTunnel] Unexpected error... \(error)")
                        self.delegate?.tunnelConnectionError(self, error: error)
                    }
                }

            } while self.listenSocket.isActive == true
        }
    }

    private func connectTunnel (from: TCPConnection, to: TCPConnection) {
        queue.async { [weak self] in
            guard let self = self else { return }
            let connection = TCPTunnelConnection(a: from, b: to)
            connection.delegate = self
            connection.start()
            self.connections.append(connection)
            self.delegate?.tunnelConnectionCreated(self, connection: connection, from: from, to: to)
        }
    }
}

extension TCPTunnel: TCPTunnelConnectionDelegate {
    func disconnected(_ connection: TCPTunnelConnection) {
        queue.async { [weak self] in
            guard let self = self else { return }
            self.connections.removeAll { $0 === connection }
            self.delegate?.tunnelConnectionDisconnected(self, connection: connection)
        }
    }
}

class LocalTCPTunnel : TCPTunnel {
    let destinationPort: Int
    init(logger: ILogger, listenPort: Int, destinationPort: Int) throws {
        logger.info("Opening tunnel from localhost:\(listenPort) -> localhost:\(destinationPort)")
        self.destinationPort = destinationPort
        try super.init(logger: logger, port: listenPort)
    }

    override func destinationConnectionProvider() throws -> Promise<TCPConnection> {
        let socket = try Socket.create()
        try socket.connect(to: "localhost", port: Int32(destinationPort))
        let tcpConnection = BlueSocketTCPConnection(logger: logger, socket: socket, queueNamePrefix: "LocalTCPTunnelDestinationConnection")
        return Promise(data: tcpConnection)
    }
}
