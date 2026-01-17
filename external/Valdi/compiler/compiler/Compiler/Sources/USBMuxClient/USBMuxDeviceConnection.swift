//
//  File.swift
//  
//
//  Created by saniul on 10/10/2019.
//

import Foundation
import BlueSocket

protocol USBMuxDeviceConnectionDelegate: AnyObject {
    func deviceConnection(_ connection: USBMuxDeviceConnection, established channel: DispatchIO)
    func deviceConnection(_ connection: USBMuxDeviceConnection, stopped result: Result<Void, Error>)
}

class USBMuxDeviceConnection {
    let deviceId: USBMuxDeviceId
    let port: Int
    weak var delegate: USBMuxDeviceConnectionDelegate?

    var channel: DispatchIO? { return connection?.channel }
    var key: String { return "usb:\(deviceId):\(port)" }
    var socket: Socket? { return connection?.socket }

    private let logger: ILogger
    private let queue: DispatchQueue
    private var connection: USBMuxConnection?

    init(logger: ILogger, deviceId: USBMuxDeviceId, port: Int) {
        self.logger = logger
        self.deviceId = deviceId
        self.port = ((port << 8) & 0xFF00) | (port >> 8)
        let queue = DispatchQueue(label: "USBMuxDeviceConnection(\(deviceId):\(port))")
        self.queue = queue
    }

    func start() {
        queue.async { [weak self] in
            self?.attemptConnecting()
        }
    }

    func stop() {
        channel?.close()
    }

    private func cleanUpAndScheduleNextAttempt() {
        self.connection = nil
        queue.asyncAfter(deadline: .now() + 1) { [weak self] in
            self?.attemptConnecting()
        }
    }

    private func attemptConnecting() {
        assert(connection == nil)

        let connection = USBMuxConnection(logger: logger, queue: queue)
        self.connection = connection

        do {
            try connection.open(onEnd: { [weak self, weak connection] result in
                guard let this = self, let connection = connection else { return }
                this.handleConnectionEnded(connection, result: result)
            })
        } catch {
            cleanUpAndScheduleNextAttempt()
            return
        }

        let packet = USBMuxPacket.packetPayloadWithRequestType(.connect, contents: [
            "DeviceID": deviceId.deviceId,
            "PortNumber": port
        ])

        connection.sendRequest(packet, callback: { [weak self, weak connection] result in
            guard let this = self else { return }

            guard case .success = result else {
                this.cleanUpAndScheduleNextAttempt()
                return
            }

            guard let channel = connection?.channel else {
                fatalError("Channel should exist by now")
            }

            this.delegate?.deviceConnection(this, established: channel)
        })
    }

    private func handleConnectionEnded(_ connection: USBMuxConnection, result: Result<Void, Error>) {
        guard self.connection === connection else {
            // Irrelevant connection
            return
        }
        delegate?.deviceConnection(self, stopped: result)
        cleanUpAndScheduleNextAttempt()
    }
}

extension USBMuxDeviceConnection: CustomStringConvertible {
    var description: String { return key }
}
