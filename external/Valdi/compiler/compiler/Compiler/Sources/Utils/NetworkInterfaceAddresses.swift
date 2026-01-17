//
//  NetworkInterfaceAddresses.swift
//  Compiler
//
//  Created by saniul on 15/05/2019.
//

import Foundation

// Use InterfaceAddresses.getAllInterfaceAddresses() to access the in_addr structures for all network interface addresses
// that will clean itself up when the InterfaceAddresses deinitializes.
final class NetworkInterfaceAddresses {
    var cInAddrs: [in_addr]
    private var pointerToFree: UnsafeMutablePointer<ifaddrs>?

    static func getAll() -> NetworkInterfaceAddresses {
        var firstInterface: UnsafeMutablePointer<ifaddrs>?
        var interfaceAddresses: [in_addr] = []

        let success = getifaddrs(&firstInterface)

        if success == 0 {
            var nextInterface: UnsafeMutablePointer<ifaddrs>? = firstInterface
            while let interface = nextInterface?.pointee {
                if interface.ifa_addr.pointee.sa_family == UInt8(AF_INET) {
                    let addressPointer = interface.ifa_addr.withMemoryRebound(to: sockaddr_in.self, capacity: 1) { $0 }
                    interfaceAddresses.append(addressPointer.pointee.sin_addr)
                }
                nextInterface = interface.ifa_next
            }
        }

        return NetworkInterfaceAddresses(pointerToFree: firstInterface, cInAddrs: interfaceAddresses)
    }

    private init(pointerToFree: UnsafeMutablePointer<ifaddrs>?, cInAddrs: [in_addr]) {
        self.pointerToFree = pointerToFree
        self.cInAddrs = cInAddrs
    }

    deinit {
        freeifaddrs(pointerToFree)
    }
}
