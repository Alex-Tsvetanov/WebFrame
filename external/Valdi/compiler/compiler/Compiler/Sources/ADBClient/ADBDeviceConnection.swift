//
//  File.swift
//  
//
//  Created by saniul on 11/10/2019.
//

import Foundation
import BlueSocket

protocol ADBDeviceDaemonConnectionDelegate: AnyObject {
    func connection(_ adbConnection: ADBDeviceDaemonConnection, established connection: DaemonTCPConnection)
    func connection(_ adbConnection: ADBDeviceDaemonConnection, stopped result: Result<Void, Error>)
}

class ADBDeviceDaemonConnection {
    weak var delegate: ADBDeviceDaemonConnectionDelegate?

    private let logger: ILogger
    let tunnel: ADBTunnel
    let queue: DispatchQueue
    private var connection: DaemonTCPConnection?

    init(logger: ILogger, tunnel: ADBTunnel) {
        self.logger = logger
        self.tunnel = tunnel
        queue = DispatchQueue(label: "ADBDeviceDaemonConnection(\(tunnel.key))")
    }

    func start() {
        queue.async { [weak self] in
            self?.attemptConnecting()
        }
    }

    func stop() {
        queue.async { [weak self] in
            self?.connection?.close()
        }
    }

    private func attemptConnecting() {
        assert(connection == nil)

        do {
            let socket = try Socket.create()
            try socket.connect(to: "localhost", port: Int32(tunnel.localPort))

            let channel = DispatchIO(type: .stream, fileDescriptor: socket.socketfd, queue: queue, cleanupHandler: { [socket] (_: Int32) in
                // Intentional retain, need to clean up the channel before we clean up the socket
                _ = socket
            })

            let connection = DaemonTCPConnection(logger: logger, key: tunnel.key, channel: channel)
            self.connection = connection
        } catch {
            cleanUpAndScheduleNextAttempt()
            return
        }

        guard let connection = self.connection else {
            return
        }

        connection.addDisconnectHandler({ [weak self, weak connection] error in
            guard let this = self, let connection = connection else { return }
            let result: Result = error.map { .failure($0) } ?? .success(())
            this.handleConnectionEnded(connection, result: result)
        })
        // TODO: change to logInfo when updated to not fail in a loop while app is not running on connected device
        logger.verbose("Localhost reloader connection with ADB device \(tunnel.deviceId) established")
        // TODO: only call established once we've confirmed that the connection can be read from

        // Can do something like, manage the connection here first, keep attempting to read the initial bytes
        // (without failing the connection if we read zero bytes) and once we do - buffer that data, and only _then_
        // hand over the DaemonTCPConnection.
        delegate?.connection(self, established: connection)
    }

    private func cleanUpAndScheduleNextAttempt() {
        self.connection = nil
        queue.asyncAfter(deadline: .now() + 1) { [weak self] in
            self?.attemptConnecting()
        }
    }

    private func handleConnectionEnded(_ connection: DaemonTCPConnection, result: Result<Void, Error>) {
        guard self.connection === connection else {
            // Irrelevant connection
            return
        }
        // TODO: change to logInfo when updated to not fail in a loop while app is not running on connected device
        logger.verbose("Localhost reloader connection with ADB device \(tunnel.deviceId) ended")
        delegate?.connection(self, stopped: result)
        cleanUpAndScheduleNextAttempt()
    }
}

extension ADBDeviceDaemonConnection: CustomStringConvertible {
    var description: String { return "ADBDeviceDaemonConnection(\(tunnel)) "}
}
