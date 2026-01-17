package com.snap.valdi.utils

import androidx.annotation.Keep
import com.snapchat.client.valdi.NativeBridge
import com.snapchat.client.valdi.utils.NativeHandleWrapper
import com.snap.valdi.utils.Disposable

@Keep
open class NativeRef(handle: Long): NativeHandleWrapper(handle), Disposable {

    override fun destroyHandle(handle: Long) {
        NativeBridge.releaseNativeRef(handle)
    }

    override fun dispose() {
        destroy()
    }

}