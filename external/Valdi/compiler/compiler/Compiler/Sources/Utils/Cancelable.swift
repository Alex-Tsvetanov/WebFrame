//
//  Cancelable.swift
//  SwiftUtils-MacOS
//
//  Created by Simon Corsin on 11/8/17.
//  Copyright © 2017 Snap Inc. All rights reserved.
//

import Foundation

/**
 Encapsulate an operation that can be canceled
 */
public protocol Cancelable {

    func cancel()

}

/**
 Encapsulate a block that can be run only once and can be canceled.
 */
public class CancelableBlock: Cancelable {

    private var block: (() -> Void)?

    public init(block: @escaping () -> Void) {
        self.block = block
    }

    public func run() {
        if let block = block {
            self.block = nil
            block()
        }
    }

    public func cancel() {
        block = nil
    }

}

/**
 Encapsulate a callback that will be called only once when cancel() is called.
 */
public class CallbackCancelable: Cancelable {

    private var onCancel: (() -> Void)?

    public init(onCancel: @escaping () -> Void) {
        self.onCancel = onCancel
    }

    public func cancel() {
        if let onCancel = onCancel {
            self.onCancel = nil
            onCancel()
        }
    }
}

/**
 Cancelable object that does nothing on cancel()
 */
public class NoOpCancelable: Cancelable {

    public func cancel() {
        // no-op
    }

    public init() {

    }

}

/**
 Cancelable that just holds a boolean state to know whether cancel() has been called or not.
 */
public class StateCancelable: Cancelable {

    private(set) public var isCanceled = false

    public func cancel() {
        isCanceled = true
    }

    public init() {

    }

}

/**
 Thread-safe Cancelable that can contains a list of Cancelable objects.
 */
public class CancelableGroup: Cancelable {

    private(set) public var isCanceled = false
    private let queue = DispatchQueue(label: "com.snapchat.cancelable_group")
    private var children = [Cancelable]()

    public init() {

    }

    public func cancel() {
        var childrenToCall = [Cancelable]()
        queue.sync {
            guard !self.isCanceled else { return }
            self.isCanceled = true
            childrenToCall = self.children
        }
        childrenToCall.forEach { $0.cancel() }
    }

    public func add(_ cancelable: Cancelable) {
        var alreadyCanceled = false
        queue.sync {
            if self.isCanceled {
                alreadyCanceled = true
            } else {
                self.children.append(cancelable)
            }
        }

        if alreadyCanceled {
            cancelable.cancel()
        }
    }

}
