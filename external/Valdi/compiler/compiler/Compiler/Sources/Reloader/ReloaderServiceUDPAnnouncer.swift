//
//  ReloaderServiceAnnouncer.swift
//  Compiler
//
//  Created by saniul on 03/02/2019.
//  Copyright © 2019 Snap Inc. All rights reserved.
//

import Foundation
import BlueSocket
import LegibleError

private let valdiMulticastGroup: String = "226.1.1.1"
private let announcerTimeInterval: TimeInterval = 5.0

/// Uses UDP broadcast to announce the addresses and ports where the reloader service is listening
final class ReloaderServiceUDPAnnouncer: ReloaderServiceAnnouncerType {
    private let logger: ILogger
    private let processID: String
    private let deviceWhitelist: ReloaderDeviceWhitelist
    private let servicePort: Int

    private let sendToAddresses: [Socket.Address]
    private let sendQueue: DispatchQueue
    private let recvQueue: DispatchQueue
    private var recvSocket: Socket
    private var announcerTimer: Timer?

    private var socketsByInterface: [in_addr: Socket] = [:]
    private var announceFailureLogger: NonSpammyLogger<AnnounceFailureKey>

    init(logger: ILogger, processID: String, deviceWhitelist: ReloaderDeviceWhitelist, servicePort: Int) throws {
        self.logger = logger
        self.processID = processID
        self.deviceWhitelist = deviceWhitelist
        self.servicePort = servicePort
        self.announceFailureLogger = NonSpammyLogger<AnnounceFailureKey>(logger: logger)

        let whitelistedAddresses = deviceWhitelist.ipAddresses.compactMap { try? $0.toIpAddress() }
        let ipAddresses = whitelistedAddresses + [try "127.0.0.1".toIpAddress()] + [try valdiMulticastGroup.toIpAddress(), UInt32.max]
        self.sendToAddresses = ipAddresses.map { Socket.Address(address: $0, port: UInt16(Ports.reloaderServiceDiscovery)) }

        self.sendQueue = DispatchQueue(label: "com.snap.valdi.ReloaderServiceAnnouncer.send")
        self.recvQueue = DispatchQueue(label: "com.snap.valdi.ReloaderServiceAnnouncer.recv")
        self.recvSocket = try Socket.create(family: .inet, type: .datagram, proto: .udp)

        try configureRecvSocket()

        if #available(OSX 10.12, *) {
            announcerTimer = Timer.scheduledTimer(withTimeInterval: announcerTimeInterval, repeats: true) { [weak self] (_) in
                logger.verbose("Reloader: Announcing 'Daemon awake' on a timer")
                try? self?.announceDaemonAwake(additionalAddress: nil)
            }
        }
        try announceDaemonAwake(additionalAddress: nil)

        startListeningForClientAwakes()
    }

    deinit {
        announcerTimer?.invalidate()
        recvSocket.close()
    }

    private func configureSendSocket(_ socket: Socket, networkInterfaceAddress: in_addr) throws {
        var addr = networkInterfaceAddress
        let result = setsockopt(socket.socketfd, Int32(IPPROTO_IP), IP_MULTICAST_IF, &addr, socklen_t(MemoryLayout<in_addr>.size))

        try socket.udpBroadcast(enable: true)

        if result < 0 {
            throw CompilerError("Failed to configure specific multicast interface for send socket")
        }
    }

    private func configureRecvSocket() throws {
        let groupIpAddr = try valdiMulticastGroup.toIpAddress()

        var mreq: ip_mreq = ip_mreq(imr_multiaddr: in_addr(s_addr: groupIpAddr), imr_interface: in_addr(s_addr: INADDR_ANY))
        let result = setsockopt(recvSocket.socketfd, Int32(IPPROTO_IP), IP_ADD_MEMBERSHIP, &mreq, socklen_t(MemoryLayout<ip_mreq>.size))

        if result < 0 {
            throw CompilerError("Failed to add multicast group membership for recv socket")
        }
    }

    private func startListeningForClientAwakes() {
        recvQueue.async { [weak self, recvSocket] in
            repeat {
                guard let this = self else {
                    break
                }

                do {
                    var data = Data()
                    let (bytesRead, address) = try this.recvSocket.listen(forMessage: &data, on: Ports.reloaderServiceDiscovery)
                    guard bytesRead > 0 else {
                        continue
                    }

                    let payload = try Valdi_DaemonServiceDiscoveryPayload(serializedData: data)
                    this.logger.verbose("Received discovery payload")

                    guard let payloadMessage = payload.message else {
                        this.logger.verbose("Couldn't decode payload message, ignoring")
                        continue
                    }

                    guard case .clientAwakeMessage(let message) = payloadMessage else {
                        this.logger.verbose("Not a 'Client awake' message, ignoring")
                        continue
                    }

                    this.logger.verbose("Client awake payload username: \(message.username) deviceId: \(message.deviceID)")
                    guard this.deviceWhitelist.usernames.contains(message.username) || this.deviceWhitelist.deviceIds.contains(message.deviceID) else {
                        this.logger.verbose("'Client awake' message does not contain whitelisted username or deviceID, ignoring")
                        continue
                    }

                    this.logger.verbose("Announcing 'Daemon awake' in response to a 'Client awake' message")
                    try this.announceDaemonAwake(additionalAddress: address)
                } catch {
                    this.logger.error("Listening loop error: \(error.legibleLocalizedDescription)")
                }
            } while recvSocket.isActive
        }
    }

    private func announceDaemonAwake(additionalAddress: Socket.Address?) throws {
        let sendToAddresses = self.sendToAddresses + [additionalAddress].compactMap({ $0 })

        let interfaceAddresses = NetworkInterfaceAddresses.getAll()

        var message = Valdi_DaemonAwakeMessage()
        message.deviceIds = Array(deviceWhitelist.deviceIds)
        message.usernames = Array(deviceWhitelist.usernames)
        message.processID = processID
        message.serviceAddresses = interfaceAddresses.cInAddrs.map { String(cString: inet_ntoa($0)) }
        message.servicePort = Int32(servicePort)
        var payload = Valdi_DaemonServiceDiscoveryPayload()
        payload.daemonAwakeMessage = message
        let payloadData = try payload.serializedData()

        sendQueue.async { [weak self] in
            guard let this = self else { return }

            this.updateInterfaces(interfaceAddresses.cInAddrs)
            for (interface, socket) in this.socketsByInterface {
                for addressToSendTo in sendToAddresses {
                    guard case let .ipv4(addr) = addressToSendTo else {
                        continue
                    }

                    let targetAddressString = String(cString: inet_ntoa(addr.sin_addr))
                    let interfaceAddressString = String(cString: inet_ntoa(interface))

                    this.logger.verbose("Sending daemon awake packet addressed to \(targetAddressString) via interface \(interfaceAddressString)")

                    do {
                        try socket.write(from: payloadData, to: addressToSendTo)
                    } catch let error {
                        let key = AnnounceFailureKey(networkInterfaceAddress: interfaceAddressString, error: error)
                        this.announceFailureLogger.log(level: .verbose, maxCount: 1, key: key, "Failed to announce daemon awake addressed to \(targetAddressString) via interface \(interfaceAddressString): \(error.legibleLocalizedDescription)")
                    }
                }
            }
        }
    }

    private func updateInterfaces(_ interfaces: [in_addr]) {
        let next = Set(interfaces)
        let current = Set(socketsByInterface.keys)

        let removedInterfaces = current.subtracting(next)
        for removedInterface in removedInterfaces {
            let interfaceAddress = String(cString: inet_ntoa(removedInterface))
            logger.verbose("Removed interface, closing socket and cleaning up: \(interfaceAddress)")

            let socket = socketsByInterface[removedInterface]
            socket?.close()
            socketsByInterface[removedInterface] = nil

            announceFailureLogger.reset(matching: { $0.networkInterfaceAddress == interfaceAddress })
        }

        let addedInterfaces = next.subtracting(current)
        for addedInterface in addedInterfaces {
            do {
                let interfaceAddress = String(cString: inet_ntoa(addedInterface))
                logger.verbose("Added interface, configuring socket for it: \(interfaceAddress)")
                let socket = try Socket.create(family: .inet, type: .datagram, proto: .udp)
                try self.configureSendSocket(socket, networkInterfaceAddress: addedInterface)
                socketsByInterface[addedInterface] = socket
            } catch {
                logger.error("Failed to configure send socket: \(error.legibleLocalizedDescription)")
            }
        }
    }
}

struct AnnounceFailureKey: Hashable {
    let networkInterfaceAddress: String
    let error: Error

    static func == (lhs: AnnounceFailureKey, rhs: AnnounceFailureKey) -> Bool {
        return (lhs.networkInterfaceAddress == rhs.networkInterfaceAddress)
            && (lhs.error.legibleDescription == rhs.error.legibleLocalizedDescription)
    }

    func hash(into hasher: inout Hasher) {
        hasher.combine(networkInterfaceAddress)
        hasher.combine(error.legibleLocalizedDescription)
    }
}
