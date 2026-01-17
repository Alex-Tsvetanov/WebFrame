package com.snap.valdi.utils

import java.util.concurrent.locks.Lock
import java.util.concurrent.locks.ReentrantLock
import kotlin.concurrent.withLock

/**
 * An AsyncWorkScheduler which will limit how many async work items can be executed at the same time.
 */
class ThrottlerAsyncWorkerScheduler(private val maxConcurrentOperations: Int): IAsyncWorkScheduler {

    private class PendingOperation(val lock: Lock, var work: ((onComplete: () -> Unit) -> Unit)?): Disposable {

        var started = false

        override fun dispose() {
            lock.withLock {
                this.work = null
            }
        }
    }

    private var currentOperationsCount = 0
    private var operations: MutableList<PendingOperation> = arrayListOf()
    private val lock: Lock = ReentrantLock()

    private fun onOperationFinished(operation: PendingOperation) {
        lock.withLock {
            operations.remove(operation)
            currentOperationsCount--
            flushLoadOperations()
        }
    }

    private fun executeNext(): Boolean {
        if (currentOperationsCount >= maxConcurrentOperations) {
            return false
        }

        val it = operations.iterator()
        while (it.hasNext()) {
            val operation = it.next()
            if (operation.started) {
                continue
            }

            val work = operation.work

            if (work == null) {
                it.remove()
                continue
            }

            operation.started = true
            currentOperationsCount++
            work {
                onOperationFinished(operation)
            }

            return true
        }

        return false
    }

    private fun flushLoadOperations() {
        while (executeNext()) {
            //
        }
    }

    override fun scheduleWork(work: (onComplete: () -> Unit) -> Unit): Disposable {
        val operation = PendingOperation(lock, work)

        lock.withLock {
            this.operations.add(operation)
            flushLoadOperations()
        }

        return operation
    }
}
