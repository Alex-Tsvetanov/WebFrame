package com.snap.valdi

import com.snap.valdi.schema.ValdiUntypedClass
import com.snapchat.client.valdi.utils.CppObjectWrapper

@ValdiUntypedClass
interface ViewFactory {
    fun getHandle(): CppObjectWrapper
}
