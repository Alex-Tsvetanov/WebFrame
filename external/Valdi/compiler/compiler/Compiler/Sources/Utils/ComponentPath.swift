//
//  File.swift
//  
//
//  Created by Simon Corsin on 12/17/19.
//

import Foundation

struct ComponentPath: Hashable, Codable {
    let fileName: String
    let exportedMember: String

    var stringRepresentation: String {
        return "\(exportedMember)@\(fileName)"
    }
}
