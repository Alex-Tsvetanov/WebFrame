//
//  MeasureExecution.swift
//  
//
//  Created by saniul on 21/10/2019.
//

import Foundation

func measureExecution(logger: ILogger, name: String, block: () throws -> Void) rethrows {
    logger.debug("\(name)...")
    let start = AbsoluteTime.current
    try block()
    let timeInterval = start.timeElapsed
    logger.debug("\(name) took: \(timeInterval)s")
}
