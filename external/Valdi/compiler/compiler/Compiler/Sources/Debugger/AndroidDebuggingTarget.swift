//
//  File.swift
//  
//
//  Created by Saniul Ahmed on 19/06/2020.
//

import Foundation

struct AndroidDebuggingTarget: Codable {
    let deviceId: String
    let target: String // host:port
}
