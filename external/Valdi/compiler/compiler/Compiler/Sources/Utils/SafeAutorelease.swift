//
//  SafeAutorelease.swift
//  
//
//  Created by Simon Corsin on 3/5/21.
//

import Foundation

@_transparent
func safeAutorelease<T>(_ block: () throws -> T) rethrows -> T {
  #if canImport(Darwin)
    return try autoreleasepool {
      try block()
    }
  #else
    return try block()
  #endif
}
