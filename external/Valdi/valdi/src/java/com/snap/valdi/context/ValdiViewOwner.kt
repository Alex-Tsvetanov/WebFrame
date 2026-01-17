package com.snap.valdi.context

import android.content.Context
import android.view.View
import com.snap.valdi.nodes.ValdiViewNode

/**
 * A ValdiViewOwner can provide views on behalf of Valdi and get called back when a render has
 * finished.
 */
interface ValdiViewOwner {
    /**
     * Called whenever a render has finished with this root view.
     * This will be called when the view is loaded the first time, when the view model is set
     * and applied, and on hot reload.
     * @param rootView
     */
    fun onRendered(rootView: View)
}