//
//  DaemonServiceConnectedClient.swift
//  Compiler
//
//  Created by Simon Corsin on 2/15/19.
//  Copyright © 2019 Snap Inc. All rights reserved.
//

import Foundation

final class DaemonServiceConnectedClient: CustomStringConvertible {

    let protocolConnection: ValdiDaemonProtocolConnection

    let id: Int
    var applicationId: String?
    var platform: Platform?
    var wasConfigured = false
    var clientIsReady = false
    var hotReloadDisabled = false
    var logsWriter: LogsWriter?
    var autoConnector: DaemonServiceAutoConnector
    var deviceId: String
    var tunnels: [TCPTunnel] = []
    var debuggerConnected = false

    init(id: Int, deviceId: String, protocolConnection: ValdiDaemonProtocolConnection, autoConnector: DaemonServiceAutoConnector) {
        self.id = id
        self.protocolConnection = protocolConnection
        self.deviceId = deviceId
        self.autoConnector = autoConnector
    }

    var description: String {
        if wasConfigured {
            return "{ platform: \(platform?.rawValue ?? ""), applicationId: \(applicationId ?? ""), connectionKey: \(protocolConnection.connectionKey) }"
        } else {
            return "{ UNCONFIGURED CLIENT \(protocolConnection.connectionKey) }"
        }
    }

}
