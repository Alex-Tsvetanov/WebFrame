package com.snap.valdi.logger

import android.util.Log

class DefaultLogger: Logger {

    private val tag = "Valdi"

    override fun log(level: Int, message: String?) {
        val unwrappedMessage = message ?: ""
        when (level) {
            LogLevel.DEBUG -> Log.d(tag, unwrappedMessage)
            LogLevel.ERROR -> Log.e(tag, unwrappedMessage)
            LogLevel.INFO -> Log.i(tag, unwrappedMessage)
            LogLevel.WARN -> Log.w(tag, unwrappedMessage)
            else -> Log.d(tag, unwrappedMessage)
        }
    }

    override fun log(level: Int, err: Throwable?, message: String?) {
        when (level) {
            LogLevel.DEBUG -> Log.d(tag, message, err)
            LogLevel.ERROR -> Log.e(tag, message, err)
            LogLevel.INFO -> Log.i(tag, message, err)
            LogLevel.WARN -> Log.w(tag, message, err)
            else -> Log.d(tag, message, err)
        }
    }
}
