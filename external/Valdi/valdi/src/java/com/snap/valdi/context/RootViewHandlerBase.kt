package com.snap.valdi.context

import com.snap.valdi.ViewRef
import com.snap.valdi.utils.ViewRefSupport
import com.snap.valdi.views.ValdiRootView
import com.snap.valdi.views.ValdiView

abstract class RootViewHandlerBase(val viewRefSupport: ViewRefSupport): IRootViewHandler {

    private var ref: ViewRef? = null

    override var rootView: ValdiRootView?
        get() = ref?.get() as? ValdiRootView
        set(value) {
            if (value != ref?.get()) {
                val previous = ref
                if (value != null) {
                    ref = ViewRef(value, false, viewRefSupport)
                } else {
                    ref = null
                }
                onRootViewChanged(ref, previous)
            }
        }

    protected abstract fun onRootViewChanged(ref: ViewRef?, previous: ViewRef?)

}