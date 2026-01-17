package com.snap.valdi.context

import com.snap.valdi.ViewRef
import com.snap.valdi.nativebridge.ContextNative
import com.snap.valdi.snapdrawing.SnapDrawingOptions
import com.snap.valdi.snapdrawing.SnapDrawingRenderMode
import com.snap.valdi.snapdrawing.SnapDrawingRuntime
import com.snap.valdi.utils.ViewRefSupport
import com.snap.valdi.views.ValdiRootView
import com.snap.valdi.views.snapdrawing.SnapDrawingContainerView

class SnapDrawingRootViewHandler(private val contextNative: ContextNative,
                                 private val snapDrawingOptions: SnapDrawingOptions,
                                 private val snapDrawingRuntime: SnapDrawingRuntime,
                                 viewRefSupport: ViewRefSupport): RootViewHandlerBase(viewRefSupport) {

    override fun onRootViewChanged(ref: ViewRef?, previous: ViewRef?) {
        val previousRootView = previous?.get() as? ValdiRootView
        val previousSnapDrawingRootHandle = (previousRootView?.snapDrawingContainerView as? SnapDrawingContainerView)?.snapDrawingRootHandle?.nativeHandle ?: 0L

        var newSnapDrawingRootHandle = 0L

        val newRootView = ref?.get() as? ValdiRootView
        if (newRootView != null) {
            var snapDrawingContainerView = newRootView.snapDrawingContainerView as? SnapDrawingContainerView
            if (snapDrawingContainerView == null) {
                snapDrawingContainerView = SnapDrawingContainerView(snapDrawingOptions, viewRefSupport.logger, snapDrawingRuntime, newRootView.context)
                newRootView.snapDrawingContainerView = snapDrawingContainerView
            }

            snapDrawingContainerView.snapDrawingOptions = snapDrawingOptions

            newSnapDrawingRootHandle = snapDrawingContainerView.snapDrawingRootHandle.nativeHandle
        }

        contextNative.setSnapDrawingRootView(newSnapDrawingRootHandle, previousSnapDrawingRootHandle, ref)
    }


}
