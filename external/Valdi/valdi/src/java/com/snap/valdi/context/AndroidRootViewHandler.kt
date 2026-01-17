package com.snap.valdi.context

import com.snap.valdi.ViewRef
import com.snap.valdi.nativebridge.ContextNative
import com.snap.valdi.utils.ViewRefSupport

class AndroidRootViewHandler(private val contextNative: ContextNative, viewRefSupport: ViewRefSupport): RootViewHandlerBase(viewRefSupport) {

    override fun onRootViewChanged(ref: ViewRef?, previous: ViewRef?) {
        contextNative.setRootView(ref)
    }

}