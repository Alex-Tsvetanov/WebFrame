//
//  File.swift
//  
//
//  Created by Simon Corsin on 9/28/20.
//

import Foundation

struct CompilationCacheEntry<T> {
    let lastSequence: CompilationSequence
    let value: T
}

class CompilationCache<T> {

    private var entries = Synchronized(data: [String: CompilationCacheEntry<T>]())

    func getOrUpdate(key: String, compileSequence: CompilationSequence, onUpdate: (String, T?) throws -> T) rethrows -> T {
        return try entries.data { (entries) -> T in
            var previousValue: T?
            if let entry = entries[key] {
                previousValue = entry.value
                if entry.lastSequence == compileSequence {
                    return entry.value
                }
            }

            let value = try onUpdate(key, previousValue)
            entries[key] = CompilationCacheEntry(lastSequence: compileSequence, value: value)
            return value
        }
    }
}
