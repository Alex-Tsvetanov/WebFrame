//
//  PropertyNameAllocator+Kotlin.swift
//  Compiler
//
//  Created by Simon Corsin on 7/26/19.
//

import Foundation

extension PropertyNameAllocator {

    class func forSwift() -> PropertyNameAllocator {
        let allocator = PropertyNameAllocator()

        // Reserve the Swift keywords
        SwiftValidation.swiftKeywords.forEach { _ = allocator.allocate(property: $0) }

        return allocator
    }

}
