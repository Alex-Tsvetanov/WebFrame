package com.snap.valdi.attributes

import androidx.annotation.Keep
import com.snap.valdi.utils.NativeRef

@Keep
class ViewLayoutAttributesCpp(handle: Long): NativeRef(handle), ViewLayoutAttributes {

    override fun getValue(attributeName: String): Any? {
        return nativeGetValue(nativeHandle, attributeName)
    }

    override fun getBoolValue(attributeName: String): Boolean {
        return nativeGetBoolValue(nativeHandle, attributeName)
    }

    override fun getStringValue(attributeName: String): String? {
        return getValue(attributeName) as? String
    }

    override fun getDoubleValue(attributeName: String): Double {
        return nativeGetDoubleValue(nativeHandle, attributeName)
    }

    companion object {
        @JvmStatic
        private external fun nativeGetValue(ptr: Long, attributeName: String): Any?

        @JvmStatic
        private external fun nativeGetBoolValue(ptr: Long, attributeName: String): Boolean

        @JvmStatic
        private external fun nativeGetDoubleValue(ptr: Long, attributeName: String): Double
    }
}