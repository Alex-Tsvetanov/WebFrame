package com.snap.valdi.promise

import androidx.annotation.Keep

@Keep
interface Promise<T> {

    fun onComplete(callback: PromiseCallback<T>)

    fun cancel()
    fun isCancelable(): Boolean

    companion object {
        @JvmStatic
        fun <T> fromResolvedValue(value: T): Promise<T> {
            return ResolvedPromise(value)
        }

        @JvmStatic
        fun <T> resolve(value: T): Promise<T> {
            return ResolvedPromise(value)
        }

        @JvmStatic
        fun <T> reject(error: Throwable): Promise<T> {
            return RejectedPromise(error)
        }
    }

}
