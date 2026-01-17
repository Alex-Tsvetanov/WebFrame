package com.snap.valdi.promise

class ResolvedPromise<T>(val value: T) : Promise<T> {

    override fun onComplete(callback: PromiseCallback<T>) {
        callback.onSuccess(value)
    }

    override fun cancel() {
    }

    override fun isCancelable(): Boolean {
        return false
    }
}