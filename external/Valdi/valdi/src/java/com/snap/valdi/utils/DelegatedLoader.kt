package com.snap.valdi.utils

import java.util.concurrent.Executor

/**
 * A generic loader which can group load operations together and trigger
 * a single load command to its delegate for items with the same key.
 */
class DelegatedLoader<Key, Item>(val delegate: LoaderDelegate<Key, Item>, val executor: Executor) {

    private class LoadOperation<Item> {
        val completions = mutableListOf<LoadCompletion<Item>>()
    }

    private val pendingOperations = hashMapOf<Key, LoadOperation<Item>>()

    fun load(key: Key, completion: LoadCompletion<Item>): Disposable {
        var requiresLoad = false
        var disposable: Disposable

        synchronized(this) {
            var loadOperation = pendingOperations[key]
            if (loadOperation == null) {
                loadOperation = LoadOperation()
                pendingOperations[key] = loadOperation

                requiresLoad = true
            }

            val disposableCompletion = DisposableLoadCompletion(completion)
            disposable = disposableCompletion
            loadOperation.completions.add(disposableCompletion)
        }

        if (requiresLoad) {
            executor.execute {
                delegate.load(key, object: LoadCompletion<Item> {
                    override fun onSuccess(item: Item) {
                        notifySuccess(key, item)
                    }

                    override fun onFailure(error: Throwable) {
                        notifyFailure(key, error)
                    }
                })
            }
        }

        return disposable
    }

    fun cancel(key: Key) {
        removeLoadOperation(key)
    }

    private fun removeLoadOperation(key: Key): LoadOperation<Item>? {
        return synchronized(this) {
            pendingOperations.remove(key)
        }
    }

    private fun notifySuccess(key: Key, item: Item) {
        val operation = removeLoadOperation(key) ?: return
        operation.completions.forEach { it.onSuccess(item) }
    }

    private fun notifyFailure(key: Key, error: Throwable) {
        val operation = removeLoadOperation(key) ?: return
        operation.completions.forEach { it.onFailure(error) }
    }



}