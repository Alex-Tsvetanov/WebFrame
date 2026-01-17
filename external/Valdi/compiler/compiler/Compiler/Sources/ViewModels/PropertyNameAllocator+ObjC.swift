//
//  PropertyNameAllocator+ObjC.swift
//  Compiler
//
//  Created by Simon Corsin on 7/30/19.
//

import Foundation

extension PropertyNameAllocator {

    class func forObjC() -> PropertyNameAllocator {
        let allocator = PropertyNameAllocator()

        // Reserve the Kotlin keywords
        ObjCValidation.objcKeywords.forEach { _ = allocator.allocate(property: $0) }

        return allocator
    }

}
