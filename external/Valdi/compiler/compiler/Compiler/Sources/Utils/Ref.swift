//
//  Ref.swift
//  Compiler
//
//  Created by Simon Corsin on 12/7/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

import Foundation

class Ref<T> {
    var value: T

    init(value: T) {
        self.value = value
    }
}

struct WeakRef<T: AnyObject> {
    weak var value: T?

    init(value: T) {
        self.value = value
    }
}
