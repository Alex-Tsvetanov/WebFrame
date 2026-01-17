package com.snap.valdi.promise

import androidx.annotation.Keep

@Keep
interface PromiseCallback<T> {
    fun onSuccess(value: T)
    fun onFailure(error: Throwable)
}