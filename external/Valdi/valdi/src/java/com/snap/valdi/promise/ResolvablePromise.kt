package com.snap.valdi.promise

import com.snap.valdi.utils.DisposableUtils

open class ResolvablePromise<T>: Promise<T> {

    private var completions: MutableList<PromiseCallback<T>>? = null
    private var completed = false
    private var error: Throwable? = null
    private var value: T? = null
    private var canceled = false

    fun fulfillSuccess(value: T) {
        var completions: MutableList<PromiseCallback<T>>?
        synchronized(this) {
            if (canceled || completed) {
                return
            }
            completed = true
            this.value = value
            completions = this.completions
            this.completions = null
        }

        completions?.forEach {
            it.onSuccess(value)
            DisposableUtils.disposeAny(it)
        }
    }

    fun fulfillFailure(error: Throwable) {
        var completions: MutableList<PromiseCallback<T>>?
        synchronized(this) {
            if (canceled || completed) {
                return
            }
            completed = true
            this.error = error
            completions = this.completions
            this.completions = null
        }

        completions?.forEach {
            it.onFailure(error)
            DisposableUtils.disposeAny(it)
        }
    }

    override fun onComplete(callback: PromiseCallback<T>) {
        var error: Throwable?
        var value: T?
        synchronized(this) {
            if (canceled) {
                return
            }

            if (!completed) {
                var completions = this.completions
                if (completions == null) {
                    completions = mutableListOf()
                    this.completions = completions
                }
                completions.add(callback)
                return
            } else {
                error = this.error
                value = this.value
                // We are already completed
                // Unlock the mutex and call the completion with the result
            }
        }

        if (error != null) {
            callback.onFailure(error!!)
        } else {
            callback.onSuccess(value!!)
        }
        DisposableUtils.disposeAny(callback)
    }

    override fun cancel() {
        doCancel()
    }

    override fun isCancelable(): Boolean {
        return false
    }

    protected fun doCancel(): Boolean {
        synchronized(this) {
            if (canceled) {
                return false
            }
            this.canceled = true
        }
        return true
    }

}
