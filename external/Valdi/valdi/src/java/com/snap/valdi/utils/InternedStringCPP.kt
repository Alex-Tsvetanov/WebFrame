package com.snap.valdi.utils

import com.snapchat.client.valdi.utils.NativeHandleWrapper

internal class InternedStringCPP: NativeHandleWrapper, InternedString {

    constructor(str: String): this(str, true)

    constructor(str: String, createHandleLazily: Boolean): super(0) {
        if (createHandleLazily) {
            javaString = str
            needsHandle = true
        } else {
            javaString = null
            needsHandle = false
            nativeHandle = nativeCreate(str)
        }
    }

    constructor(handle: Long): super(handle) {
        javaString = null
        needsHandle = false
        nativeRetain(handle)
    }

    // We keep a reference to the Java string until we actually need to create the interned string
    private var javaString: String?
    private var needsHandle: Boolean

    override fun toString(): String {
        return javaString ?: nativeToString(nativeHandle)
    }

    fun cacheJavaString() {
        if (javaString == null) {
            javaString = nativeToString(nativeHandle)
        }
    }

    override fun destroyHandle(handle: Long) {
        nativeRelease(handle)
    }

    override fun getNativeHandle(): Long {
        if (needsHandle) {
            synchronized(this) {
                if (needsHandle) {
                    needsHandle = false
                    val javaString = this.javaString
                    if (javaString != null) {
                        this.javaString = null
                        nativeHandle = nativeCreate(javaString)
                    }
                }
            }
        }

        return super.getNativeHandle()
    }

    override fun hashCode(): Int {
        return nativeHandle.toInt()
    }

    override fun equals(other: Any?): Boolean {
        if (other !is InternedStringCPP) {
            return false
        }
        return nativeHandle == other.nativeHandle
    }

    companion object {
        @JvmStatic
        private external fun nativeCreate(str: String): Long
        @JvmStatic
        private external fun nativeRelease(ptr: Long)

        @JvmStatic
        private external fun nativeToString(ptr: Long): String

        @JvmStatic
        private external fun nativeRetain(ptr: Long)
    }
}
