//
//  TransformedResource.swift
//  Compiler
//
//  Created by Simon Corsin on 6/12/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

import Foundation

struct ImageResource {
    let outputFilename: String
    let file: File
    let imageScale: Double
    let isRemote: Bool
}

struct DocumentResource {
    let outputFilename: String
    let file: File
}
