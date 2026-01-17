package com.snap.valdi.exceptions

/**
 * Exception thrown whenever there is an error applying Valdi's attributes.
 * Valdi will take care of showing debug informations when this error is thrown,
 * so you don't need to specify the attribute name or attribute value in the message.
 */
class AttributeError(message: String): ValdiException(message)
