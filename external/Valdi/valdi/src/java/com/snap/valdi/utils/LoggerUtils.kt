package com.snap.valdi.utils

import com.snap.valdi.logger.LogLevel
import com.snap.valdi.logger.Logger

fun Logger.debug(message: String?) {
    log(LogLevel.DEBUG, message)
}

fun Logger.info(message: String?) {
    log(LogLevel.INFO, message)
}

fun Logger.error(message: String?) {
    log(LogLevel.ERROR, message)
}

fun Logger.warn(message: String?) {
    log(LogLevel.WARN, message)
}