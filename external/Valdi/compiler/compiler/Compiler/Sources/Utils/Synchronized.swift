//
//  Synchronized.swift
//  Compiler
//
//  Created by Simon Corsin on 12/10/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

import Foundation

class Synchronized<T> {

    var unsafeData: T {
        return lock.lock {
            innerData
        }
    }

    private var innerData: T
    private let lock = DispatchSemaphore.newLock()

    init(data: T) {
        self.innerData = data
    }

    func data(_ closure: (inout T) throws -> Void) rethrows {
        try lock.lock {
            try closure(&innerData)
        }
    }

    func data<Out>(_ closure: (inout T) throws -> Out) rethrows -> Out {
        return try lock.lock {
            return try closure(&innerData)
        }
    }

}
