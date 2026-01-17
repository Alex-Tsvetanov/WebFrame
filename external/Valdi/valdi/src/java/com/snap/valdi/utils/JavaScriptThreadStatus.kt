package com.snap.valdi.utils

enum class JavaScriptThreadStatus(val status: Integer) {
    NOT_RUNNING(Integer(0)),
    RUNNING(Integer(1)),
    TIMED_OUT(Integer(2));

    companion object {
        fun fromCpp(status: Integer) = JavaScriptThreadStatus.values().first { it.status == status }
    }
}