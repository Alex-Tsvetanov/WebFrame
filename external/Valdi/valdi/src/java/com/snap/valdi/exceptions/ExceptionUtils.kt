package com.snap.valdi.exceptions

import java.lang.StringBuilder

private fun buildMessageWithCauses(throwable: Throwable, builder: StringBuilder) {
    val message = throwable.message
    builder.append(throwable::class.java.simpleName)
    builder.append(": '")
    builder.append(message ?: "Unknown Error")
    builder.append("'")

    val cause = throwable.cause
    if (cause != null) {
        builder.append(", Caused by: ")
        buildMessageWithCauses(cause, builder)
    }
}

fun Throwable.messageWithCauses(): String {
    val sb = StringBuilder()

    buildMessageWithCauses(this, sb)

    return sb.toString()
}