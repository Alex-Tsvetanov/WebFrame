//
//  File.swift
//
//
//  Created by saniul on 11/10/2019.
//

import Foundation

class DaemonServiceADBAutoConnector: ADBDeviceDaemonConnectionDelegate, DaemonServiceAutoConnector {
    weak var delegate: DaemonServiceAutoConnectorDelegate?

    private let logger: ILogger
    private let companion: CompanionExecutable
    private let client: ADBClient
    private let queue = DispatchQueue(label: "ADBClient")

    private var attachedDevices: Set<ADBDeviceId> = []
    private var connections: [ADBTunnel: ADBDeviceDaemonConnection] = [:]
    private var openTunnels: Set<ADBTunnel> = []
    private var requestedTunnels: Set<ADBTunnel> = []

    init(logger: ILogger, companion: CompanionExecutable) {
        self.logger = logger
        self.companion = companion
        self.client = ADBClient(logger: logger)
    }

    func start() {
        queue.async { [weak self] in
            self?.doStart()
        }
    }

    func createTunnel(logger: ILogger, fromLocalPort localPort: Int, toDeviceId: String, devicePort: Int) throws -> TCPTunnel? {
        try? self.client.closeTunnelForDevice(ADBTunnel(deviceId: ADBDeviceId(serial: toDeviceId), localPort: localPort, devicePort: devicePort))

        logger.info("Opening ADB Tunnel from localhost:\(localPort) -> \(toDeviceId):\(devicePort)")
        let newTunnel = try client.openTunnelForDevice(ADBDeviceId(serial: toDeviceId), devicePort: devicePort, localPort: localPort)

        queue.async { [weak self] in
            self?.requestedTunnels.insert(newTunnel)
        }
        return nil
    }

    private func doStart() {
        logger.info("Started listening for ADB devices")

        let openReloaderTunnels = (try? client.getCurrentlyOpenTunnels(matchingDevicePort: Ports.reloaderOverUSB)) ?? []
        let openDebuggerTunnels = (try? client.getCurrentlyOpenTunnels(matchingDevicePort: Ports.valdiDebugger)) ?? []
        let tunnels = Array(openReloaderTunnels) + Array(openDebuggerTunnels)
        for tunnel in tunnels {
            try? client.closeTunnelForDevice(tunnel)
        }
        update()
    }

    private func update() {
        logAttachedDetachedDevices()

        var openReloaderTunnels = (try? client.getCurrentlyOpenTunnels(matchingDevicePort: Ports.reloaderOverUSB)) ?? []
        var openDebuggerTunnels = (try? client.getCurrentlyOpenTunnels(matchingDevicePort: Ports.valdiDebugger)) ?? []
        let openTunnels = openReloaderTunnels.union(openDebuggerTunnels)
        let tunnelsByDeviceId = openReloaderTunnels.groupBy { $0.deviceId }

        let devicesWithoutTunnels = attachedDevices.filter { !tunnelsByDeviceId.keys.contains($0) }
        for deviceId in devicesWithoutTunnels {
            do {
                let openedReloaderTunnel = try client.openTunnelForDevice(deviceId, devicePort: Ports.reloaderOverUSB)
                openReloaderTunnels.insert(openedReloaderTunnel)
                let openedDebuggerTunnel = try client.openTunnelForDevice(deviceId, devicePort: Ports.valdiDebugger)
                openDebuggerTunnels.insert(openedDebuggerTunnel)
            } catch {
                logger.error("Could not open tunnel for ADB device \(deviceId): \(error.legibleLocalizedDescription)")
            }
        }

        let freshlyClosedTunnels = self.openTunnels.subtracting(openTunnels)
        let newlyOpenTunnels = openTunnels.subtracting(self.openTunnels)
        self.openTunnels = openTunnels

        for closedTunnel in freshlyClosedTunnels {
            let connection = connections.removeValue(forKey: closedTunnel)
            connection?.stop()
        }

        for newTunnel in newlyOpenTunnels where newTunnel.devicePort == Ports.reloaderOverUSB {
            let connection = ADBDeviceDaemonConnection(logger: logger, tunnel: newTunnel)
            connections[newTunnel] = connection
            connection.delegate = self
            connection.start()
        }

        let debuggingTargets = openDebuggerTunnels.map { tunnel in
            return AndroidDebuggingTarget(deviceId: tunnel.deviceId.serial,
                                          target: "localhost:\(tunnel.localPort)")
        }

        _ = companion.updatedAndroidTargets(targets: debuggingTargets)

        queue.asyncAfter(deadline: .now() + 1) { [weak self] in
            self?.update()
        }
    }

    private func logAttachedDetachedDevices() {
        let attachedDevices = (try? client.getCurrentlyAttachedDevices()) ?? []
        let freshlyDetachedDevices = self.attachedDevices.subtracting(attachedDevices)
        let newlyAttachedDevices = attachedDevices.subtracting(self.attachedDevices)
        self.attachedDevices = attachedDevices

        for detachedDevice in freshlyDetachedDevices {
            logger.info("ADB device detached: \(detachedDevice)")
        }

        for attachedDevice in newlyAttachedDevices {
            logger.info("ADB device attached: \(attachedDevice)")
        }
    }

    // MARK: DaemonServiceAutoConnectorDelegate

    func connection(_ adbConnection: ADBDeviceDaemonConnection, established connection: DaemonTCPConnection) {
        logger.debug("ADBDeviceDaemonConnection connection \(adbConnection) established")
        queue.async { [weak self] in
            guard let this = self else { return }
            this.delegate?.autoConnector(this, newConnectionEstablished: connection, deviceId: adbConnection.tunnel.deviceId.serial)
        }
    }

    func connection(_ adbConnection: ADBDeviceDaemonConnection, stopped result: Result<Void, Error>) {
        logger.debug("ADBDeviceDaemonConnection connection \(adbConnection) stopped with result: \(result)")

        queue.async { [weak self] in
            guard let this = self else { return }

            guard this.connections[adbConnection.tunnel] === adbConnection else {
                return
            }

            for tunnel in this.openTunnels where tunnel.deviceId == adbConnection.tunnel.deviceId {
                try? this.client.closeTunnelForDevice(tunnel)
            }
            for tunnel in this.requestedTunnels where tunnel.deviceId == adbConnection.tunnel.deviceId {
                try? this.client.closeTunnelForDevice(tunnel)
                this.requestedTunnels.remove(tunnel)
            }
            this.connections[adbConnection.tunnel] = nil
        }
    }
}
