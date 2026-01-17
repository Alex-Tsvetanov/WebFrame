//
//  PipelineItems.swift
//  Compiler
//
//  Created by Simon Corsin on 7/24/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

import Foundation

struct SelectedItem<T> {
    let item: CompilationItem
    let data: T
}

struct GroupedItems<Key, T> {
    let key: Key
    let items: [T]
}

struct SelectedCompilationItems<T> {
    let compileSequence: CompilationSequence
    let unselectedItems: [CompilationItem]
    let selectedItems: [T]

    func discardSelected() -> CompilationItems {
        return CompilationItems(compileSequence: compileSequence, items: unselectedItems)
    }

    func discardNonSelected() -> SelectedCompilationItems<T> {
        return SelectedCompilationItems(compileSequence: compileSequence, unselectedItems: [], selectedItems: self.selectedItems)
    }

    func transformEach(_ eachClosure: (T) throws -> Void) rethrows -> CompilationItems {
        return try transformEach { (item) -> [CompilationItem] in
            try eachClosure(item)
            return []
        }
    }

    func transformEach(_ eachClosure: (T) throws -> CompilationItem) rethrows -> CompilationItems {
        return try transformEach {
            [try eachClosure($0)]
        }
    }

    func transformEachConcurrently(_ eachClosure: (T) throws -> CompilationItem) rethrows -> CompilationItems {
        return try transformAll { (items) -> [CompilationItem] in
            return try items.parallelMap { (item) in
                try eachClosure(item)
            }
        }
    }

    func transformEachConcurrently(_ eachClosure: (T) throws -> [CompilationItem]) rethrows -> CompilationItems {
        return try transformAll { (items) -> [CompilationItem] in
            return try items.parallelMap { (item) in
                try eachClosure(item)
                }.flatMap { $0 }
        }
    }

    @discardableResult func processEachConcurrently(_ eachClosure: (T) throws -> Void) rethrows -> SelectedCompilationItems<T> {
        try selectedItems.parallelProcess { (item) in
            try eachClosure(item)
        }

        return self
    }

    @discardableResult func processAll(_ allClosure: ([T]) throws -> Void) rethrows -> SelectedCompilationItems<T> {
        try allClosure(selectedItems)
        return self
    }

    func transformEach(_ eachClosure: (T) throws -> [CompilationItem]) rethrows -> CompilationItems {
        var newItems = [CompilationItem]()
        newItems.reserveCapacity(unselectedItems.count + selectedItems.count)
        newItems.append(contentsOf: unselectedItems)

        for selectedItem in selectedItems {
            let updatedItems = try eachClosure(selectedItem)
            newItems += updatedItems
        }

        return CompilationItems(compileSequence: compileSequence, items: newItems)
    }

    func transformAll(_ allClosure: ([T]) throws -> [CompilationItem]) rethrows -> CompilationItems {
        let updatedItems = try allClosure(selectedItems)

        return CompilationItems(compileSequence: compileSequence, items: unselectedItems + updatedItems)
    }

    func transformInBatches(_  batchClosure: (_ items: [T]) throws -> [CompilationItem]) rethrows -> CompilationItems {
        return try transformInBatches(ProcessInfo.processInfo.activeProcessorCount, batchClosure)
    }

    func transformInBatches(_ numberOfBatches: Int, _ batchClosure: (_ items: [T]) throws -> [CompilationItem]) rethrows -> CompilationItems {
        let distance = Int(ceil(Float(selectedItems.count) / Float(numberOfBatches)))
        let batchSequence = BatchSequence(array: selectedItems, distance: distance)
        let batches = Array(batchSequence)
        let processedBatches = try batches.parallelMap(batchClosure)
        let updatedItems = processedBatches.flatMap { $0 }

        return CompilationItems(compileSequence: compileSequence, items: unselectedItems + updatedItems)
    }

    func groupBy<Key: Hashable>(_ getKeyClosure: (T) -> Key) -> SelectedCompilationItems<GroupedItems<Key, T>> {
        var itemsByKey = [Key: [T]]()
        for item in selectedItems {
            let key = getKeyClosure(item)
            itemsByKey[key, default: []].append(item)
        }

        let newSelectedItems = itemsByKey.map { GroupedItems(key: $0.key, items: $0.value) }

        return SelectedCompilationItems<GroupedItems<Key, T>>(compileSequence: compileSequence, unselectedItems: unselectedItems, selectedItems: newSelectedItems)
    }
}

struct CompilationItems {

    let compileSequence: CompilationSequence
    var allItems: [CompilationItem]

    init(compileSequence: CompilationSequence, items: [CompilationItem]) {
        self.compileSequence = compileSequence
        self.allItems = items
    }

    mutating func append(item: CompilationItem) {
        self.allItems.append(item)
    }

    func select<T>(_ selectClosure: (CompilationItem) -> T?) -> SelectedCompilationItems<SelectedItem<T>> {
        var unselectedItems = [CompilationItem]()
        unselectedItems.reserveCapacity(allItems.count)

        var selectedItems = [SelectedItem<T>]()

        for item in allItems {
            if let data = selectClosure(item) {
                selectedItems.append(SelectedItem(item: item, data: data))
            } else {
                unselectedItems.append(item)
            }
        }

        return SelectedCompilationItems(compileSequence: compileSequence, unselectedItems: unselectedItems, selectedItems: selectedItems)
    }

    func selectAll() -> SelectedCompilationItems<CompilationItem> {
        return SelectedCompilationItems(compileSequence: compileSequence, unselectedItems: [], selectedItems: allItems)
    }

    func debugLog(logger: ILogger) {
        for item in allItems {
            logger.error(item.toString())
        }
    }

}
