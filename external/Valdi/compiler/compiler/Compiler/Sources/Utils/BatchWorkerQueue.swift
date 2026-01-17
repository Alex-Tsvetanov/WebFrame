//
//  BatchWorkerQueue.swift
//  
//
//  Created by Simon Corsin on 2/12/20.
//

import Foundation

/**
 A generic worker queue which can process items
 in batch as they are added.
 */
class BatchWorkerQueue<In, Out> {

    private struct WorkItem {
        let item: In
        let promise: Promise<Out>
    }

    private let queue: DispatchQueue
    private let processor: ([In]) throws -> [Result<Out, Error>]
    private let lock = DispatchSemaphore.newLock()
    private var workItems: [WorkItem] = []

    init(queue: DispatchQueue, processor: @escaping ([In]) throws -> [Result<Out, Error>]) {
        self.queue = queue
        self.processor = processor
    }

    func enqueue(item: In) -> Promise<Out> {
        let promise = Promise<Out>()

        lock.lock {
            self.workItems.append(WorkItem(item: item, promise: promise))
        }

        queue.async {
            self.process()
        }

        return promise
    }

    private func dequeueAllItems() -> [WorkItem] {
        return lock.lock {
            let items = self.workItems
            self.workItems = []
            return items
        }
    }

    private func process() {
        let itemsToProcess = self.dequeueAllItems()

        guard !itemsToProcess.isEmpty else { return }

        let items = itemsToProcess.map { $0.item }

        do {
            let results = try processor(items)
            guard results.count == items.count else {
                throw CompilerError("Processor should have returned \(items.count) results instead of \(results.count)")
            }

            for (index, result) in results.enumerated() {
                itemsToProcess[index].promise.fulfill(result: result)
            }
        } catch let error {
            itemsToProcess.forEach { (workItem) in
                workItem.promise.reject(error: error)
            }
        }

    }
}
