//
//  PropertyNameAllocator+Kotlin.swift
//  Compiler
//
//  Created by Simon Corsin on 7/26/19.
//

import Foundation

extension PropertyNameAllocator {

    class func forKotlin() -> PropertyNameAllocator {
        let allocator = PropertyNameAllocator()

        // Reserve the Kotlin keywords
        KotlinValidation.kotlinKeywords.forEach { _ = allocator.allocate(property: $0) }

        // Reserve builtin functions
        _ = allocator.allocate(property: "toString")
        _ = allocator.allocate(property: "copy")

        return allocator
    }

}
