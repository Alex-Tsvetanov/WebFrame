package com.snap.valdi.exceptions

import androidx.annotation.Keep

/**
 * Exception thrown whenever there is an error applying an attribute.
 */
@Keep
class MarshallerException(message: String): ValdiException(message)
