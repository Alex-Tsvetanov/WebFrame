package com.snap.valdi.utils

internal class InternedStringJava(val str: String): InternedString {

    override fun toString(): String {
        return str
    }

}