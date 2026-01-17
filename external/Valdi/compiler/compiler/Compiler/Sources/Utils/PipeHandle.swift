//
//  PipeHandle.swift
//  Compiler
//
//  Created by Simon Corsin on 4/12/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

import Foundation

public class PipeHandle: FileHandleReader {

    public let pipe = Pipe()

    private var buffer = Data()

    public init(dispatchQueue: DispatchQueue?) {
        super.init(fileHandle: pipe.fileHandleForReading, dispatchQueue: dispatchQueue)
    }
}
