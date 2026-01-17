package com.snap.valdi.utils

import com.snapchat.client.valdi.NativeBridge

open class CppNativeHandlePair(protected var nativeHandle1: Long, protected var nativeHandle2: Long): Disposable {

    protected fun finalize() {
        doDispose()
    }

    protected fun swapNativeHandle1(): Long {
        val nativeHandle1 = this.nativeHandle1
        this.nativeHandle1 = 0L
        return nativeHandle1
    }

    protected fun swapNativeHandle2(): Long {
        val nativeHandle2 = this.nativeHandle2
        this.nativeHandle2 = 0L
        return nativeHandle2
    }

    private fun doDispose() {
        val nativeHandle1 = this.swapNativeHandle1()
        if (nativeHandle1 != 0L) {
            NativeBridge.releaseNativeRef(nativeHandle1)
        }
        val nativeHandle2 = this.swapNativeHandle2()
        if (nativeHandle2 != 0L) {
            NativeBridge.releaseNativeRef(nativeHandle2)
        }
    }

    override fun dispose() {
        doDispose()
    }
}