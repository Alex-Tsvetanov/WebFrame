//
//  ReloaderContext.swift
//  Compiler
//
//  Created by saniul on 28/12/2018.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

import Foundation

struct ReloaderDeviceWhitelist {
    init(deviceIds: Set<String>, usernames: Set<String>, ipAddresses: Set<String>) {
        self.deviceIds = deviceIds
        self.usernames = usernames

        var foundAddressPorts = Set<AddressPort>()
        var foundIpAddresses = Set<String>()
        for addressStr in ipAddresses {
            let components = addressStr.components(separatedBy: ":")
            if components.count == 2, let port = Int32(components[1]) {
                let address = components[0]
                let addressPort = AddressPort(family: .IPv4, address: address, port: port)
                foundAddressPorts.insert(addressPort)
                foundIpAddresses.insert(address)
            } else {
                foundIpAddresses.insert(addressStr)
            }
        }
        self.ipAddresses = foundIpAddresses
        addressPorts = foundAddressPorts
    }

    let deviceIds: Set<String>
    let usernames: Set<String>
    let ipAddresses: Set<String>
    let addressPorts: Set<AddressPort>
}
