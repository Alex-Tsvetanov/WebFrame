//
//  PropertyNameAllocator.swift
//  Compiler
//
//  Created by Simon Corsin on 12/19/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

import Foundation

struct PropertyName: CustomStringConvertible {

    var name: String {
        if id == 0 {
            return originalName
        }

        return "\(originalName)\(id + 1)"
    }

    let originalName: String
    let id: Int

    init(originalName: String, id: Int) {
        self.originalName = originalName
        self.id = id
    }

    init(name: String) {
        self.originalName = name
        self.id = 0
    }

    var description: String {
        return name
    }

}

/**
 PropertyNameAllocator can generate identifiers and
 ensure they don't conflict. It can be scoped so that
 identifiers can be allocated without impacting the
 parent scope.
 */
class PropertyNameAllocator {

    private(set) weak var parent: PropertyNameAllocator?

    private var propertyNames = [String: Set<Int>]()

    func allocate(property: String) -> PropertyName {
        let propertyName = resolve(property: property)

        propertyNames[propertyName.originalName, default: []].insert(propertyName.id)

        return propertyName
    }

    private func resolve(property: String) -> PropertyName {
        var firstId = 0
        if let parent = parent {
            // If we have a parent, the property should not conflict with anything it has
            firstId = parent.resolve(property: property).id
        }

        let existingIds = propertyNames[property, default: []]

        while existingIds.contains(firstId) {
            firstId += 1
        }

        return PropertyName(originalName: property, id: firstId)
    }

    /**
     Returns a new NameAllocator which will have its own scope
     for the allocated properties, while ensuring the scoped properties
     don't conflict with this name allocator as well.
     */
    func scoped() -> PropertyNameAllocator {
        let nameAllocator = PropertyNameAllocator()
        nameAllocator.parent = self
        return nameAllocator
    }
}
