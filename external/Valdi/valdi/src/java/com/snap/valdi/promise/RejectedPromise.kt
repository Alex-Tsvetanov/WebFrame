package com.snap.valdi.promise

class RejectedPromise<T>(val error: Throwable) : Promise<T> {

    override fun onComplete(callback: PromiseCallback<T>) {
        callback.onFailure(error)
    }

    override fun cancel() {
    }

    override fun isCancelable(): Boolean {
        return false
    }
}
