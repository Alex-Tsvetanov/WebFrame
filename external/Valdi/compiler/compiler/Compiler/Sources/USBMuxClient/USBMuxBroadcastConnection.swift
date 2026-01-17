//
//  File.swift
//  
//
//  Created by saniul on 10/10/2019.
//

import Foundation

protocol USBMuxBroadcastConnectionDelegate: AnyObject {
    func broadcastConnection(_ connection: USBMuxBroadcastConnection, started result: Result<Void, Error>)
    func broadcastConnection(_ connection: USBMuxBroadcastConnection, stopped result: Result<Void, Error>)

    func broadcastConnection(_ connection: USBMuxBroadcastConnection, deviceAttached deviceId: USBMuxDeviceId)
    func broadcastConnection(_ connection: USBMuxBroadcastConnection, deviceDetached deviceId: USBMuxDeviceId)
}

private enum USBMuxBroadcastResponseType: String {
    case deviceAttached = "Attached"
    case deviceDetached = "Detached"
}

class USBMuxBroadcastConnection {
    weak var delegate: USBMuxBroadcastConnectionDelegate?

    private let logger: ILogger
    private let connection: USBMuxConnection
    private let queue: DispatchQueue

    init(logger: ILogger) {
        self.logger = logger
        let queue = DispatchQueue(label: "USBMuxBroadcastConnection")
        self.queue = queue
        let connection = USBMuxConnection(logger: logger, queue: queue)
        self.connection = connection
    }

    func start() {
        do {
            try connection.open(onEnd: { [weak self] result in
                guard let this = self else { return }
                this.delegate?.broadcastConnection(this, stopped: result)
            })
            connection.broadcastHandler = { [weak self] packet in
                self?.handleBroadcastPacket(packet)
            }

            let packet = USBMuxPacket.packetPayloadWithRequestType(.listen, contents: [:])
            connection.sendRequest(packet, callback: { [weak self] result in
                guard let this = self else { return }
                this.delegate?.broadcastConnection(this, started: result)
            })
        } catch {
            delegate?.broadcastConnection(self, started: .failure(error))
            return
        }
    }

    func handleBroadcastPacket(_ packet: USBMuxPlistPayload) {
        guard let messageType = packet["MessageType"] as? String else {
            logger.warn("Warning: Broadcast message with missing MessageType: \(packet)")
            return
        }

        guard let broadcastResponseType = USBMuxBroadcastResponseType(rawValue: messageType) else {
            logger.warn("Warning: Unknown broadcast message: \(packet)")
            return
        }

        switch broadcastResponseType {
        case .deviceAttached:
            guard let deviceIdInt = packet["DeviceID"] as? Int else {
                logger.warn("Warning: Received 'Attached' message, but couldn't parse: \(packet)")
                return
            }
            let deviceId = USBMuxDeviceId(deviceIdInt)
            delegate?.broadcastConnection(self, deviceAttached: deviceId)
        case .deviceDetached:
            guard let deviceIdInt = packet["DeviceID"] as? Int else {
                logger.warn("Warning: Received 'Detached' message, but couldn't parse: \(packet)")
                return
            }
            let deviceId = USBMuxDeviceId(deviceIdInt)
            delegate?.broadcastConnection(self, deviceDetached: deviceId)
        }
    }
}
