package com.snap.valdi.utils

interface InternedString {

    companion object {
        fun create(str: String): InternedString {
            return if (ValdiJNI.available) {
                InternedStringCPP(str)
            } else {
                InternedStringJava(str)
            }
        }
    }
}
