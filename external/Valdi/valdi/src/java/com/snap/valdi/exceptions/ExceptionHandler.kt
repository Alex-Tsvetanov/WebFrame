package com.snap.valdi.exceptions

interface ExceptionHandler {

    fun onExceptionThrown(exception: Throwable, isFatal: Boolean)

}
