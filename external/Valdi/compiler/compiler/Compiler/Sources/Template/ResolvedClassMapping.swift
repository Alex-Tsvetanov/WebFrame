//
//  CurrentClassMapping.swift
//  Compiler
//
//  Created by Simon Corsin on 8/12/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

import Foundation

// TODO: rename, something like CapturedClassMapping or CompoundClassMapping
struct ResolvedClassMapping {

    let localClassMapping: ValdiClassMapping?
    let projectClassMapping: ProjectClassMapping
    let currentBundle: CompilationItem.BundleInfo

    func resolve(nodeType: String) throws -> ValdiClass {
        return try projectClassMapping.resolveClass(nodeType: nodeType,
                                                    currentBundle: currentBundle,
                                                    localMapping: localClassMapping)
    }

}
