//
//  DaemonServiceUSBMuxAutoConnector.swift
//  Compiler
//
//  Created by saniul on 09/10/2019.
//

import Foundation

final class DaemonServiceUSBMuxAutoConnector: DaemonServiceAutoConnector, USBMuxClientDelegate {
    weak var delegate: DaemonServiceAutoConnectorDelegate?

    private let logger: ILogger
    private let queue = DispatchQueue(label: "USBMuxAutoConnector", attributes: .concurrent)
    private let usbMuxClient: USBMuxClient

    init(logger: ILogger) {
        self.logger = logger
        let usbMuxClient = USBMuxClient(logger: logger, devicePortNumber: Ports.reloaderOverUSB)
        self.usbMuxClient = usbMuxClient
        usbMuxClient.delegate = self
    }

    func start() {
        usbMuxClient.start()
    }

    func usbMuxClientDeviceConnected(_ client: USBMuxClient, deviceId: USBMuxDeviceId, connection: USBMuxDeviceConnection) {
        logger.info("USB reloader connection with iOS device established: \(deviceId)")

        guard let channel = connection.channel else {
            fatalError("usbMuxDeviceConnected, but no channel")
        }

        let connectionKey = "usb:\(deviceId):\(connection.port)"
        let connection = DaemonTCPConnection(logger: logger, key: connectionKey, channel: channel)
        delegate?.autoConnector(self, newConnectionEstablished: connection, deviceId: deviceId.description)
    }

    func usbMuxClientDeviceDisconnected(_ client: USBMuxClient, deviceId: USBMuxDeviceId) {
        queue.async(flags: .barrier) {
            self.logger.info("USB reloader connection with iOS device ended: \(deviceId)")
        }
    }

    func createTunnel(logger: ILogger, fromLocalPort localPort: Int, toDeviceId: String, devicePort: Int) throws -> TCPTunnel? {
        let usbMuxDeviceId = USBMuxDeviceId(Int(toDeviceId)!)
        logger.info("Opening USBMux Tunnel from localhost:\(localPort) -> DeviceId:\(usbMuxDeviceId):\(devicePort)")
        return try USBMuxTunnel(logger: logger, listenPort: localPort, deviceId: usbMuxDeviceId, devicePort: devicePort)
    }
}
