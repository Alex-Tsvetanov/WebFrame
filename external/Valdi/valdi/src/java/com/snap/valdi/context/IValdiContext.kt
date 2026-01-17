package com.snap.valdi.context

import com.snap.valdi.modules.drawing.Size
import com.snap.valdi.nodes.IValdiViewNode
import com.snap.valdi.views.ValdiRootView

interface IValdiContext {
    val contextId: ValdiContextId
    var rootView: ValdiRootView?
    var viewModel: Any?

    /**
     * Asynchronously wait until all the pending updates of this ValdiContext have completed, and
     * call the given callback when done. If there are no pending updates in this ValdiContext,
     * the callback will be called immediately.
     * If the ValdiContext is Lazy, this will trigger the render
     */
    fun waitUntilAllUpdatesCompleted(callback: () -> Unit)

    /**
     * Enqueues a callback that will be executed once a complete layout pass has finished.
     * Will be called immediately if the layout is up to date.
     */
    fun onNextLayout(callback: () -> Unit)

    /**
    Schedule an exclusive update function, which will be called once
    all the pending exclusive updates have finished running. This can also be used to batch
    ViewNode setAttribute operations.
     */
    fun scheduleExclusiveUpdate(callback: Runnable)

    /**
     * Perform a FlexBox layout calculation using the given width and height measure specs.
     */
    fun measureLayout(widthMeasureSpec: Int, heightMeasureSpec: Int, isRTL: Boolean): Size

    /**
     * Set the layout size and direction for the component. This will potentially
     * calculate the layout and update the view hierarchy.
     */
    fun setLayoutSpecs(width: Int, height: Int, isRTL: Boolean)

    /**
     * Set the visible viewport that should be used when computing the viewports from
     * the root node. This can be used to only render a smaller area of the root node.
     * When not set, the root node will be considered as completely visible.
     */
    fun setVisibleViewport(x: Int, y: Int, width: Int, height: Int)

    /**
     * Unset a previously set visible viewport. The root node will be considered
     * as completely visible.
     */
    fun unsetVisibleViewport()

    /**
     * Returns the associated ValdiViewNode for the given viewId, which is the value set in id="someId".
     */
    fun getViewNode(viewId: String): IValdiViewNode?

    /**
     * Returns the associated ValdiViewNode for the given viewNodeId, which is a stable identifier
     * generated at runtime that uniquely identifies the node.
     */
    fun getViewNodeForId(viewNodeId: Int): IValdiViewNode?

    /**
     * Returns the root ViewNode
     */
    fun getRootViewNode(): IValdiViewNode?

    /**
    * Set whether the layout sepcs should be kept when the layout is invalidated. When this option
    * is set, the underlying Valdi Context will be able to immediately recalculate the layout on
    * layout invalidation without asking platform about the new layout specs. You can use this when
    * the root view's size is not dependent on calculating the size of the Valdi tree.
    */
    fun setRetainsLayoutSpecsOnInvalidateLayout(retainsLayoutSpecsOnInvalidateLayout: Boolean)

    /**
     * Enqueue a callback to call on the next render
     */
    fun enqueueNextRenderCallback(onNextRenderCallback: () -> Unit)

    /**
     * Call destroy once you are sure that this Valdi node tree is not needed anymore and should release all
     * possible resources.
     */
    fun destroy()

    /**
     * Returns whether this Valdi Context was destroyed.
     */
    fun isDestroyed(): Boolean
}
