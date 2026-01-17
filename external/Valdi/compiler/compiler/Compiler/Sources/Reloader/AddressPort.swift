//
//  AddressPort.swift
//  BlueSocket
//
//  Created by Simon Corsin on 6/5/19.
//

import Foundation

struct AddressPort: Hashable, CustomStringConvertible {
    enum Family: Hashable {
        case IPv4
        case IPv6
    }

    let family: Family
    let address: String
    let port: Int32

    var description: String {
        return "\(address):\(port)"
    }
}
