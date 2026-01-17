//
//  PropertyNameAllocator+CPP.swift
//  
//
//  Created by Simon Corsin on 4/11/23.
//

import Foundation

extension PropertyNameAllocator {

    class func forCpp() -> PropertyNameAllocator {
        let allocator = PropertyNameAllocator()

        // Reserve the C++ keywords
        CppValidation.cppKeywords.forEach { _ = allocator.allocate(property: $0) }

        return allocator
    }

}
