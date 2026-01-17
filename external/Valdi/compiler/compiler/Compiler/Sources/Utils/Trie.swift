// Copyright © 2024 Snap, Inc. All rights reserved.

struct Trie<T> {
    class Entry {
        typealias Name = [String]
        let name: Name
        var value: T?
        var children = [String: Entry]()

        init(name: Name, value: T?) {
            self.name = name
            self.value = value
        }
    }

    struct Result {
        let nearestName: Entry.Name?
        let nearestValue: T?
        let found: Bool
    }

    private var root = Entry(name: [], value: nil)

    func insert(name: Entry.Name, value: T) {
        upsertEntry(entry: root, name: name, index: 0).value = value
    }

    func get(name: Entry.Name) -> Result {
        return doGetEntry(name: name)
    }

    private func upsertEntry(entry: Entry, name: Entry.Name, index: Int) -> Entry {
        guard index < name.count else {
            return entry
        }

        let component = name[index]
        var resolvedEntry = entry.children[component]
        if resolvedEntry == nil {
            resolvedEntry = Entry(name: Entry.Name(name[0...index]), value: nil)
            entry.children[component] = resolvedEntry
        }

        return upsertEntry(entry: resolvedEntry!, name: name, index: index + 1)
    }

    private func doGetEntry(name: Entry.Name) -> Result {
        var currentEntry = self.root
        var bestEntry: Entry?

        if currentEntry.value != nil {
            bestEntry = currentEntry
        }

        for i in 0..<name.count {
            let component = name[i]
            guard let child = currentEntry.children[component] else {
                return Result(nearestName: bestEntry?.name, nearestValue: bestEntry?.value, found: false)
            }

            currentEntry = child
            if child.value != nil {
                bestEntry = child
            }
        }

        return Result(nearestName: bestEntry?.name, nearestValue: bestEntry?.value, found: bestEntry != nil)
    }
}
