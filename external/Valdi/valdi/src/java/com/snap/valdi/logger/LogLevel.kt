package com.snap.valdi.logger

/**
 * Contains LogLevels used by Valdi JVM and C++.
 * Those values reflect what is being used by C++.
 */
sealed class LogLevel {
    companion object {
        val DEBUG = 0
        val INFO = 1
        val WARN = 2
        val ERROR = 3
    }
}