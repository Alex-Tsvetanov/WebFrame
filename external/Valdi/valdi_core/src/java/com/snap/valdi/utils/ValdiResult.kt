package com.snap.valdi.utils

import androidx.annotation.Keep
import com.snapchat.client.valdi.utils.NativeHandleWrapper

/**
 * A opaque Result type which can be used to express a success or failure
 * to a call from C++
 */
@Keep
abstract class ValdiResult {

    @Keep
    abstract fun isSuccess(): Boolean
    @Keep
    abstract fun isFailure(): Boolean

    @Keep
    abstract fun getErrorMessage(): String
    @Keep
    abstract fun getSuccessValue(): Any?

    companion object {

        private class ValdiSuccessResult(val value: Any?): ValdiResult() {
            override fun isSuccess(): Boolean {
                return true
            }

            override fun isFailure(): Boolean {
                return false
            }

            override fun getErrorMessage(): String {
                throw AssertionError("This is not a failure result")
            }

            override fun getSuccessValue(): Any? {
                return value
            }
        }

        private class ValdiFailureResult(val error: String): ValdiResult() {

            override fun isSuccess(): Boolean {
                return false
            }

            override fun isFailure(): Boolean {
                return true
            }

            override fun getErrorMessage(): String {
                return error
            }

            override fun getSuccessValue(): Any? {
                throw AssertionError("This is not a success result")
            }
        }

        fun isSuccess(result: ValdiResult?): Boolean {
            if (result == null) {
                return true
            }

            return result.isSuccess()
        }

        fun success(): ValdiResult? {
            return success(null)
        }

        @JvmStatic
        @Keep
        fun failure(exception: Throwable): ValdiResult? {
            return failure(exception.message ?: "Exception thrown")
        }

        @JvmStatic
        @Keep
        fun success(data: Any?): ValdiResult? {
            if (data == null) {
                // To avoid additional JNI calls, we express success result with no data
                // as null.
                return null
            }

            return ValdiSuccessResult(data)
        }

        @JvmStatic
        @Keep
        fun failure(error: String): ValdiResult? {
            return ValdiFailureResult(error)
        }
    }
}