//
//  File.swift
//  
//
//  Created by Simon Corsin on 10/29/19.
//

import Foundation

struct DeclaredFileDependencies {
    let key: String
    let dependencies: Set<URL>
}

class FileDependenciesManager {

    private let lock = DispatchSemaphore.newLock()

    private var dependenciesByFile = [URL: [DeclaredFileDependencies]]()
    private var dependentsByFile = [URL: Set<URL>]()
    private var dependentsDirty = false

    func register(file: URL, dependencies: Set<URL>, forKey key: String, append: Bool = false) {
        lock.lock {
            var declaredDependencies = dependenciesByFile[file, default: []]

            if let existingIndex = declaredDependencies.firstIndex(where: { $0.key == key }) {
                let existingDeps = declaredDependencies[existingIndex].dependencies

                let newDependencies: Set<URL>
                if append {
                    newDependencies = existingDeps.union(dependencies)
                } else {
                    newDependencies = dependencies
                }

                if existingDeps == newDependencies {
                    // Dependencies are the same
                    return
                }

                declaredDependencies[existingIndex] = DeclaredFileDependencies(key: key, dependencies: newDependencies)
            } else {
                declaredDependencies.append(DeclaredFileDependencies(key: key, dependencies: dependencies))
            }

            dependenciesByFile[file] = declaredDependencies
            dependentsDirty = true
        }
    }

    private func computeDependents() {
        // TODO(simon): Can be made incremental if necessary
        dependentsByFile.removeAll()

        for (file, declaredDependencies) in dependenciesByFile {
            for dependencies in declaredDependencies {
                for dependency in dependencies.dependencies {
                    dependentsByFile[dependency, default: []].insert(file)
                }
            }
        }
    }

    func getDependents(file: URL) -> [URL] {
        return lock.lock {
            if dependentsDirty {
                dependentsDirty = false
                computeDependents()
            }
            guard let dependents = dependentsByFile[file] else {
                return []
            }
            return dependents.map { $0 }
        }
    }

}
