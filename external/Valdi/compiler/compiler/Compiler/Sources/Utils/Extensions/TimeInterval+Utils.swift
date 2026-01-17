//
//  TimeInterval+Utils.swift
//  Compiler
//
//  Created by David Byttow on 10/27/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

import Foundation

public extension TimeInterval {
    var milliseconds: Int {
        return Int(self * 1_000.0)
    }

    var microseconds: Int {
        return Int(self * 1_000_000.0)
    }

    fileprivate static let since1970: TimeInterval = 978307200
}

public typealias AbsoluteTime = Double
public extension AbsoluteTime {
    static var current: AbsoluteTime {
        #if os(macOS)
        return CFAbsoluteTimeGetCurrent()
        #else
        var tv = timeval()
        gettimeofday(&tv, nil)
        return TimeInterval(tv.tv_sec) - TimeInterval.since1970 + (1.0E-6 * TimeInterval(tv.tv_usec))
        #endif
    }

    var timeElapsed: TimeInterval {
        return AbsoluteTime.current - self
    }
}
