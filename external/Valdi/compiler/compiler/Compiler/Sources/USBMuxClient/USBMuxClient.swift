//
//  USBDeviceHub.swift
//  Compiler
//
//  Created by saniul on 02/10/2019.
//

import Foundation
import BlueSocket

protocol USBMuxClientDelegate: AnyObject {
    func usbMuxClientDeviceConnected(_ client: USBMuxClient, deviceId: USBMuxDeviceId, connection: USBMuxDeviceConnection)
    func usbMuxClientDeviceDisconnected(_ client: USBMuxClient, deviceId: USBMuxDeviceId)
}

class USBMuxClient: USBMuxBroadcastConnectionDelegate, USBMuxDeviceConnectionDelegate {
    weak var delegate: USBMuxClientDelegate?

    let devicePortNumber: Int

    private let logger: ILogger
    private var queue =  DispatchQueue(label: "USBMuxClient")

    private var broadcastConnection: USBMuxBroadcastConnection?
    private var deviceConnections: [USBMuxDeviceId: USBMuxDeviceConnection] = [:]
    private var started = false

    init(logger: ILogger, devicePortNumber: Int) {
        self.logger = logger
        self.devicePortNumber = devicePortNumber
    }

    func start() {
        logger.debug("USBMuxClient starting")

        guard !started else {
            logger.warn("USBMuxClient already started")
            return
        }
        started = true

        let broadcastConnection = USBMuxBroadcastConnection(logger: logger)
        self.broadcastConnection = broadcastConnection
        broadcastConnection.delegate = self
        broadcastConnection.start()
    }

    // MARK: USBMuxBroadcastConnectionDelegate

    func broadcastConnection(_ connection: USBMuxBroadcastConnection, started result: Result<Void, Error>) {
        switch result {
        case .success:
            logger.info("USBMuxClient started listening for USB iOS devices")
        case let .failure(error):
            logger.error("USBMuxClient failed to start: \(error.legibleLocalizedDescription)")
        }
    }

    func broadcastConnection(_ connection: USBMuxBroadcastConnection, stopped result: Result<Void, Error>) {
        switch result {
        case .success:
            logger.info("USBMuxClient stopped listening for USB iOS devices")
        case let .failure(error):
            logger.error("USBMuxClient stopped with error: \(error.legibleLocalizedDescription)")
        }
    }

    func broadcastConnection(_ connection: USBMuxBroadcastConnection, deviceAttached deviceId: USBMuxDeviceId) {
        // TODO: print device properties too?
        logger.info("USB iOS device attached: \(deviceId)")

        queue.async { [weak self, port = devicePortNumber] in
            guard let self else { return }
            let deviceConnection = USBMuxDeviceConnection(logger: self.logger, deviceId: deviceId, port: port)
            self.deviceConnections[deviceId] = deviceConnection
            deviceConnection.delegate = self
            deviceConnection.start()
        }
    }

    func broadcastConnection(_ connection: USBMuxBroadcastConnection, deviceDetached deviceId: USBMuxDeviceId) {
        logger.info("USB iOS device detached: \(deviceId)")

        queue.async { [weak self] in
            guard let deviceConnection = self?.deviceConnections.removeValue(forKey: deviceId) else {
                return
            }
            self?.delegate?.usbMuxClientDeviceDisconnected(self!, deviceId: deviceId)
            deviceConnection.stop()
        }
    }

    // MARK: USBMuxDeviceConnectionDelegate

    func deviceConnection(_ connection: USBMuxDeviceConnection, established channel: DispatchIO) {
        logger.debug("USBMuxClient connection \(connection) established")
        let deviceId = connection.deviceId

        queue.async { [weak self] in
            guard let this = self else { return }

            guard this.deviceConnections[deviceId] === connection else {
                return
            }
            this.delegate?.usbMuxClientDeviceConnected(this, deviceId: deviceId, connection: connection)
        }
    }

    func deviceConnection(_ connection: USBMuxDeviceConnection, stopped result: Result<Void, Error>) {
        logger.debug("USBMuxClient connection \(connection) stopped with result: \(result)")
        let deviceId = connection.deviceId

        queue.async { [weak self] in
            guard let this = self else { return }

            guard this.deviceConnections[deviceId] === connection else {
                return
            }

            this.delegate?.usbMuxClientDeviceDisconnected(this, deviceId: deviceId)
        }
    }
}
