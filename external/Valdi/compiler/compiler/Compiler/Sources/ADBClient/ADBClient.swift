//
//  File.swift
//  
//
//  Created by saniul on 10/10/2019.
//

import Foundation
import BlueSocket

struct ADBDeviceId: Hashable, CustomStringConvertible {
    let serial: String

    var description: String { return serial }
}

struct ADBTunnel: Hashable, CustomStringConvertible {
    let deviceId: ADBDeviceId
    let localPort: Int
    let devicePort: Int

    var key: String { return "adb:\(deviceId):\(localPort)" }

    var description: String { return key }
}

enum ADBError: Error {
    case couldNotOpenTunnel
    case couldNotParseLocalPort
}

class ADBClient {
    private let logger: ILogger

    init(logger: ILogger) {
        self.logger = logger
    }
    
    func getCurrentlyAttachedDevices() throws -> Set<ADBDeviceId> {
        let result = try runADBCommand(command: ["adb", "devices", "-l"])
        let lines = result.components(separatedBy: .newlines).filter { !$0.isEmpty }
        let deviceLines = (lines.first?.contains("List of devices") == true) ? Array(lines.dropFirst()) : lines
        let serials = deviceLines
            .compactMap { $0.components(separatedBy: .whitespaces).first }
        let deviceIds = serials.map { ADBDeviceId(serial: $0) }

        return Set(deviceIds)
    }

    func getCurrentlyOpenTunnels(matchingDevicePort: Int) throws -> Set<ADBTunnel> {
        let result = try runADBCommand(command: ["adb", "forward", "--list"])
        let lines = result.components(separatedBy: .newlines).filter { !$0.isEmpty }
        let componentsByLine = lines.map { $0.components(separatedBy: .whitespaces).filter { !$0.isEmpty } }
        let tunnels = componentsByLine.compactMap { components -> ADBTunnel? in
            guard let serial = components[safe: 0],
                let localPortString = components[safe: 1]?.components(separatedBy: ":")[safe: 1],
                let localPort = Int(localPortString),
                let devicePortString = components[safe: 2]?.components(separatedBy: ":")[safe: 1],
                let devicePort = Int(devicePortString),
                devicePort == matchingDevicePort
                else { return nil }

            let tunnel = ADBTunnel(deviceId: ADBDeviceId(serial: serial), localPort: localPort, devicePort: devicePort)
            return tunnel
        }

        return Set(tunnels)
    }

    func openTunnelForDevice(_ deviceId: ADBDeviceId, devicePort: Int) throws -> ADBTunnel {
        return try openTunnelForDevice(deviceId, devicePort: devicePort, localPort: 0)
    }

    func openTunnelForDevice(_ deviceId: ADBDeviceId, devicePort: Int, localPort: Int) throws -> ADBTunnel {
        let result = try runADBCommand(command: ["adb", "-s", deviceId.serial, "forward", "tcp:\(localPort)", "tcp:\(devicePort)"])
        let lines = result.components(separatedBy: .newlines).filter { !$0.isEmpty }
        guard let localPortLine = lines.first else {
            throw ADBError.couldNotOpenTunnel
        }
        guard let boundLocalPort = Int(localPortLine) else {
            throw ADBError.couldNotParseLocalPort
        }
        let tunnel = ADBTunnel(deviceId: deviceId, localPort: boundLocalPort, devicePort: devicePort)
        return tunnel
    }

    func closeTunnelForDevice(_ tunnel: ADBTunnel) throws {
        _ = try runADBCommand(command: ["adb", "forward", "--remove", "tcp:\(tunnel.localPort)"])
    }

    func runADBCommand(command: [String]) throws -> String {
        let process = SyncProcessHandle.usingEnv(logger: logger, command: command)
        try process.run()

        if !process.stderr.content.isEmpty {
            throw CompilerError("adb error: \(process.stderr.contentAsString)")
        }

        let outputData = process.stdout.content
        guard let outputString = String(data: outputData, encoding: .utf8) else {
            throw CompilerError("could not decode adb output: \(outputData)")
        }
        return outputString
    }
}
