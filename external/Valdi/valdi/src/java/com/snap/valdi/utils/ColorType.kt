package com.snap.valdi.utils

enum class ColorType {
    UNKNOWN,
    RGBA8888,
    BGRA8888,
    ALPHA8,
    GRAY8,
    RGBAF16,
    RGBAF32;

    val cppValue: Int
        get() = this.ordinal
}