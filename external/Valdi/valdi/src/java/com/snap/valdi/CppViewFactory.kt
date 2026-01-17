package com.snap.valdi

import com.snapchat.client.valdi.utils.CppObjectWrapper

class CppViewFactory(nativeHandle: Long): CppObjectWrapper(nativeHandle), ViewFactory {
    override fun getHandle(): CppObjectWrapper {
        return this
    }
}