//
//  BackoffTimer.swift
//  Compiler
//
//  Created by saniul on 27/12/2018.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

import Foundation

final class BackoffTimer {
    private let label: String
    private let block: () -> Void

    var minTimeInterval: TimeInterval = 0
    var maxTimeInterval: TimeInterval = 60
    var bumpModifier: (TimeInterval) -> TimeInterval = { max(1, $0) * 2 }

    lazy var currentTimeInterval: TimeInterval = { return minTimeInterval }()

    private var timer: Timer? {
        didSet {
            oldValue?.invalidate()
        }
    }

    init(label: String = "", block: @escaping () -> Void) {
        self.label = label
        self.block = block
    }

    deinit {
        timer = nil
    }

    func bump() {
        let bumpedTimeInterval = bumpModifier(currentTimeInterval)
        currentTimeInterval = min(maxTimeInterval, bumpedTimeInterval)
        if #available(OSX 10.12, *) {
            timer = Timer.scheduledTimer(withTimeInterval: currentTimeInterval, repeats: false, block: { [weak self] _ in
                self?.block()
            })
        }
    }

    func reset() {
        timer = nil
        currentTimeInterval = minTimeInterval
    }
}
