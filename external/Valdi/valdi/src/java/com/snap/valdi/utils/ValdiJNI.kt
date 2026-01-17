package com.snap.valdi.utils

object ValdiJNI {
    // We only compile the JNI code for Android for now
    val available = (System.getProperty("java.specification.vendor") ?: "").lowercase().contains("android")
}