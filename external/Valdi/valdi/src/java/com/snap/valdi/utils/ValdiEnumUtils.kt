package com.snap.valdi.utils

import com.snap.valdi.schema.ValdiValueMarshallerRegistry

object ValdiEnumUtils {
    @JvmStatic
    fun <E, T: Enum<E>> getEnumStringValue(enumCase: T): String {
        return ValdiValueMarshallerRegistry.shared.getEnumStringValue(enumCase::class.java, enumCase)
    }

    @JvmStatic
    fun <E, T: Enum<E>> getEnumIntValue(enumCase: T): Int {
        return ValdiValueMarshallerRegistry.shared.getEnumIntValue(enumCase::class.java, enumCase)
    }
}