package com.snap.valdi.nativebridge

import com.snap.valdi.ValdiRuntime
import com.snap.valdi.ViewRef
import com.snapchat.client.valdi.NativeBridge
import com.snap.valdi.utils.NativeRef
/**
 * Bridge class for a native Context, which forwards calls to the NativeBridge.
 */
class ContextNative(nativeHandle: Long, val nativeBridge: NativeBridge, val runtimeNative: RuntimeNative) {

    val nativeWrapper = NativeRef(nativeHandle)

    fun onCreate() {
        NativeBridge.contextOnCreate(nativeWrapper.nativeHandle)
    }

    fun destroy() {
        if (nativeWrapper.nativeHandle != 0L) {
            NativeBridge.destroyContext(runtimeNative.nativeHandle, nativeWrapper.nativeHandle)
            nativeWrapper.destroy()
        }
    }

    fun markDestroyed() {
        nativeWrapper.destroy()
    }

    fun getView(id: String): ViewRef? {
        return NativeBridge.getViewInContextForId(nativeWrapper.nativeHandle, id) as? ViewRef
    }

    fun setRootView(rootView: ViewRef?) {
        NativeBridge.setRootView(runtimeNative.nativeHandle, nativeWrapper.nativeHandle, rootView)
    }

    fun setSnapDrawingRootView(snapDrawingRootHandle: Long, previousSnapDrawingRootHandle: Long, rootView: ViewRef?) {
        NativeBridge.setSnapDrawingRootView(nativeWrapper.nativeHandle, snapDrawingRootHandle, previousSnapDrawingRootHandle, rootView)
    }

    fun setViewModel(viewModel: Any?) {
        NativeBridge.setViewModel(nativeWrapper.nativeHandle, viewModel)
    }

    fun callJSFunction(functionName: String, arguments: Array<Any?>) {
        NativeBridge.callJSFunction(runtimeNative.nativeHandle, nativeWrapper.nativeHandle, functionName, arguments)
    }

    fun measureLayout(maxWidth: Int, widthMode: Int, maxHeight: Int, heightMode: Int, isRTL: Boolean): Long {
        return NativeBridge.measureLayout(runtimeNative.nativeHandle, nativeWrapper.nativeHandle, maxWidth, widthMode, maxHeight, heightMode, isRTL)
    }

    fun setLayoutSpecs(width: Int, height: Int, isRTL: Boolean) {
        NativeBridge.setLayoutSpecs(runtimeNative.nativeHandle, nativeWrapper.nativeHandle, width, height, isRTL)
    }

    fun setVisibleViewport(x: Int, y: Int, width: Int, height: Int) {
        NativeBridge.setVisibleViewport(runtimeNative.nativeHandle, nativeWrapper.nativeHandle, x, y, width, height, false)
    }

    fun unsetVisibleViewport() {
        NativeBridge.setVisibleViewport(runtimeNative.nativeHandle, nativeWrapper.nativeHandle, 0, 0, 0, 0, true)
    }

    fun getRuntime(): ValdiRuntime? {
        return NativeBridge.getRuntimeAttachedObjectFromContext(nativeWrapper.nativeHandle) as? ValdiRuntime
    }

}
