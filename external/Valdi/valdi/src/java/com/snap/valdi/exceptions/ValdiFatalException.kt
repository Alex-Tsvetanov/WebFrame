package com.snap.valdi.exceptions

import androidx.annotation.Keep

/**
 * Base class for fatal Valdi exceptions.
 * ValdiFatalException should be thrown for unrecoverable errors.
 */
@Keep
open class ValdiFatalException(message: String, cause: Throwable? = null): RuntimeException(message, cause) {
    companion object {
        @JvmStatic
        fun handleFatal(throwable: Throwable): Nothing {
            GlobalExceptionHandler.onFatalException(throwable)
        }

        @JvmStatic
        fun handleFatal(throwable: Throwable, message: String): Nothing {
            val wrappedException = ValdiFatalException(message, throwable)
            handleFatal(wrappedException)
        }

        @JvmStatic
        fun handleFatal(message: String): Nothing {
            val wrappedException = ValdiFatalException(message)
            handleFatal(wrappedException)
        }
    }
}
