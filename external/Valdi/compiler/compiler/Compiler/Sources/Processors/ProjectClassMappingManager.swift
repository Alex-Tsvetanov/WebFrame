//
//  ProjectClassMappingManager.swift
//  Compiler
//
//  Created by saniul on 14/06/2019.
//

import Foundation

final class ProjectClassMappingManager {
    private var projectClassMapping: ProjectClassMapping
    private let lock = DispatchSemaphore.newLock()

    init(allowMappingOverride: Bool) {
        projectClassMapping = ProjectClassMapping(allowMappingOverride: allowMappingOverride)
    }

    func mutate(_ closure: (inout ProjectClassMapping) throws -> Void) throws {
        try lock.lock {
            try closure(&projectClassMapping)
        }
    }

    func copyProjectClassMapping() -> ProjectClassMapping {
        return lock.lock { return projectClassMapping }
    }
}
