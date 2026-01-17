//
//  USBMuxTunnel.swift
//  Compiler
//
//  Created by elee2 on 02/13/2024.
//

import Foundation

struct USBMuxTunnelPendingTCPConnection {
    let deviceConnection: USBMuxDeviceConnection
    let promise: Promise<TCPConnection>
}

class USBMuxTunnel : TCPTunnel {
    private var deviceId: USBMuxDeviceId
    private var devicePort: Int
    private var pendingTCPConnections: [USBMuxTunnelPendingTCPConnection] = []
    private let queue = DispatchQueue(label: "USBMuxTunnel")

    init(logger: ILogger, listenPort: Int, deviceId: USBMuxDeviceId, devicePort: Int) throws {        
        self.deviceId = deviceId
        self.devicePort = devicePort
        try super.init(logger: logger, port: listenPort)
    }

    override func destinationConnectionProvider() throws -> Promise<TCPConnection> {
        let promise = Promise<TCPConnection>()
        self.queue.async { [self, promise] in
            let deviceConnection = USBMuxDeviceConnection(logger: self.logger, deviceId: self.deviceId, port: self.devicePort)
            deviceConnection.delegate = self
            deviceConnection.start()
            let pendingTCPConnection = USBMuxTunnelPendingTCPConnection(deviceConnection: deviceConnection, promise: promise)
            self.pendingTCPConnections.append(pendingTCPConnection)

            self.queue.asyncAfter(deadline: .now() + 3) { [self] in
                self.pendingTCPConnections.first(where: { $0.deviceConnection === deviceConnection })?.promise.reject(error: USBMuxError.timeout)
                self.pendingTCPConnections.removeAll { $0.deviceConnection === deviceConnection }
            }
        }
        return promise
    }
}

extension USBMuxTunnel : USBMuxDeviceConnectionDelegate {
    func deviceConnection(_ deviceConnection: USBMuxDeviceConnection, established channel: DispatchIO) {
        self.queue.async { [weak self] in
            guard let self = self else { return }
            let tcpConnection = BlueSocketTCPConnection(logger: self.logger, socket: deviceConnection.socket!, queueNamePrefix: "USBMuxTunnelDeviceConnection")
            self.pendingTCPConnections.first(where: { $0.deviceConnection === deviceConnection })?.promise.resolve(data: tcpConnection)
            self.pendingTCPConnections.removeAll { $0.deviceConnection === deviceConnection }
        }
    }

    func deviceConnection(_ deviceConnection: USBMuxDeviceConnection, stopped result: Result<Void, Error>) {
    }
}
