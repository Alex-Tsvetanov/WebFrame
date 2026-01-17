//
//  ValdiUserConfig.swift
//  Compiler
//
//  Created by Simon Corsin on 2/18/19.
//  Copyright © 2019 Snap Inc. All rights reserved.
//

import Foundation
import Yams

struct ValdiUserConfig {
    var usernames: [String]
    var deviceIds: [String]
    var ipAddresses: [String]
    var logsOutputDir: URL?
    var deviceStorageDir: URL?

    static func from(configUrl: URL, args: ValdiCompilerArguments) throws -> ValdiUserConfig {
        let configContent = try String(contentsOf: configUrl)
        guard let config = try Yams.compose(yaml: configContent) else {
            throw CompilerError("Could not parse user config at: \(configUrl)")
        }

        var usernames = config["usernames"]?.array().compactMap { $0.string } ?? []
        var deviceIds = config["device_ids"]?.array().compactMap { $0.string } ?? []
        var ipAddresses = config["ip_addresses"]?.array().compactMap { $0.string } ?? []
        let logsOutputDir = config["logs_output_dir"]?.string.map { configUrl.deletingLastPathComponent().resolving(path: $0, isDirectory: true) }
        let deviceStorageDir = config["device_storage_dir"]?.string.map { configUrl.deletingLastPathComponent().resolving(path: $0, isDirectory: true) }

        // Temporary compatibility with usernames/deviceIds provided by --username --device-id
        usernames.append(contentsOf: args.username)
        deviceIds.append(contentsOf: args.deviceId)
        ipAddresses.append(contentsOf: args.ip)

        return ValdiUserConfig(usernames: usernames, deviceIds: deviceIds, ipAddresses: ipAddresses, logsOutputDir: logsOutputDir, deviceStorageDir: deviceStorageDir)
    }
}
