package com.snap.valdi.promise

abstract class CancelableResolvablePromise<T>: ResolvablePromise<T>() {

    protected abstract fun onCancel()

    override fun cancel() {
        if (doCancel()) {
            onCancel()
        }
    }

    override fun isCancelable(): Boolean {
        return true
    }
}