package com.snap.valdi.logger

import androidx.annotation.Keep

/**
 * Used by Valdi C++ and JVM to log things.
 * level is one of the com.snap.valdi.logger.LogLevel
 */
interface Logger {

    @Keep
    fun log(level: Int, message: String?)

    @Keep
    fun log(level: Int, err: Throwable?, message: String?)

    @Keep
    fun isEnabledForType(level: Int): Boolean {
        return true
    }
}