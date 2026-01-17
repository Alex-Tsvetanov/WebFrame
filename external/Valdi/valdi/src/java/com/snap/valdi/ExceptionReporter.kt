package com.snap.valdi

/**
 * stackTrace format:
 * 
 * The stack trace string consists of multiple frames, each separated by a newline.
 * Each frame follows this format:
 * "at <method> (<file>:<line>:<column>)"
 *
 * Example:
 *    at nestedCall (playground/src/Playground.tsx:36:20)
 *    at <anonymous> (playground/src/Playground.tsx:40:20)
 */

interface ExceptionReporter {
    /**
     * Reports a non-fatal error.
     *
     * @param errorCode The error code associated with the error.
     * @param message The error message.
     * @param module The module where the error occurred. Can be nil is not applicable or module is unknown.
     * @param stackTrace The stack trace of the error. Can be nil if stack trace could not be obtained. See comment above for format.
     */
    fun reportNonFatal(errorCode: Int, message: String, module: String?, stackTrace: String?)

    /**
     * Reports a crash.
     *
     * @param message The crash message.
     * @param module The module where the crash occurred.
     * @param stackTrace The stack trace of the crash. Can be nil if stack trace could not be obtained. See comment above for format.
     * @param isANR Indicates if the crash is an Application Not Responding (ANR) error.
     */
    fun reportCrash(message: String, module: String?, stackTrace: String?, isANR: Boolean)
}
