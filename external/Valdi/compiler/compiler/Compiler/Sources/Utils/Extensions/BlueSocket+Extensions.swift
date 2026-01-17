//
//  ReloaderServiceAnnouncer.swift
//  Compiler
//
//  Created by saniul on 2019-03-07.
//  Copyright © 2019 Snap Inc. All rights reserved.
//

import Foundation
import BlueSocket

extension Socket.Address {

    init(address: UInt32, port: UInt16) {
        let inaddr = in_addr(s_addr: address)
        var socketaddr = sockaddr_in()
        socketaddr.sin_family = sa_family_t(AF_INET)
        socketaddr.sin_port = in_port_t(port.bigEndian)
        socketaddr.sin_addr = inaddr
        self = Socket.Address.ipv4(socketaddr)
    }

}

extension in_addr: Equatable, Hashable {
    public static func ==(lhs: in_addr, rhs: in_addr) -> Bool {
        return lhs.s_addr == rhs.s_addr
    }

    public func hash(into hasher: inout Hasher) {
        hasher.combine(s_addr)
    }
}
