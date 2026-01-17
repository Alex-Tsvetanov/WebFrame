//
//  File.swift
//  
//
//  Created by saniul on 10/10/2019.
//

import Foundation
import BlueSocket

final class DaemonServiceSimulatorAutoConnector: DaemonServiceAutoConnector {
    weak var delegate: DaemonServiceAutoConnectorDelegate?

    let port: Int
    var key: String { return "localhost:\(port)" }

    private let logger: ILogger
    private let queue = DispatchQueue(label: "DaemonServiceSimulatorAutoConnector")
    private var socket: Socket?

    init(logger: ILogger, port: Int) {
        self.logger = logger
        self.port = port
    }

    func start() {
        queue.async { [weak self] in
            self?.attemptConnecting()
        }
    }

    private func cleanUpAndScheduleNextAttempt() {
        self.socket = nil
        queue.asyncAfter(deadline: .now() + 1) { [weak self] in
            self?.attemptConnecting()
        }
    }

    private func attemptConnecting() {
        assert(socket == nil)

        do {
            let socket = try Socket.create()
            try socket.connect(to: "localhost", port: Int32(port))
            self.socket = socket

            let connection = DaemonTCPConnection(logger: logger, socket: socket)
            connection.addDisconnectHandler({ [weak self] _ in
                guard let self else { return }
                self.logger.info("Localhost reloader connection with iOS Simulator ended")
                self.cleanUpAndScheduleNextAttempt()
            })
            logger.info("Localhost reloader connection with iOS Simulator established")
            delegate?.autoConnector(self, newConnectionEstablished: connection, deviceId: "localhost")
        } catch {
            cleanUpAndScheduleNextAttempt()
            return
        }
    }

    func createTunnel(logger: ILogger, fromLocalPort localPort: Int, toDeviceId: String, devicePort: Int) throws -> TCPTunnel? {
        return try LocalTCPTunnel(logger: logger, listenPort: localPort, destinationPort: devicePort)
    }
}
