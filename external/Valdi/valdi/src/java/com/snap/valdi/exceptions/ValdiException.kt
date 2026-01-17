package com.snap.valdi.exceptions

import androidx.annotation.Keep

/**
 * Base class for Valdi exceptions.
 * Valdi exceptions should be thrown only for recoverable errors.
 */
@Keep
open class ValdiException(message: String, cause: Throwable?): RuntimeException(message, cause) {

    @Keep
    constructor(message: String): this(message, null)

}