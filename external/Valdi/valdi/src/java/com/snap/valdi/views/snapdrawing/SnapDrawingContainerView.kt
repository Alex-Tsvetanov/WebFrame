package com.snap.valdi.views.snapdrawing

import android.content.Context
import android.graphics.Canvas
import android.graphics.Rect
import android.view.MotionEvent
import android.view.Surface
import android.view.View
import android.view.ViewGroup
import com.snap.valdi.extensions.removeFromParentView
import com.snap.valdi.snapdrawing.SnapDrawingRootHandle
import com.snap.valdi.ViewRef
import com.snap.valdi.logger.Logger
import com.snap.valdi.snapdrawing.SnapDrawingOptions
import com.snap.valdi.snapdrawing.SnapDrawingRuntime
import com.snap.valdi.snapdrawing.SnapDrawingSurfacePresenterListener
import com.snap.valdi.snapdrawing.SurfacePresenterFactory
import com.snap.valdi.snapdrawing.SurfacePresenterManager
import com.snap.valdi.utils.error
import java.lang.ref.WeakReference

open class SnapDrawingContainerView(
        var snapDrawingOptions: SnapDrawingOptions,
        private val logger: Logger,
        private val runtime: SnapDrawingRuntime,
        context: Context): ViewGroup(context) {

    val snapDrawingRootHandle: SnapDrawingRootHandle = runtime.createRoot(!snapDrawingOptions.enableSynchronousDraw)

    private var ignoreRequestLayoutCount = 0
    private val childLayoutParams = LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.MATCH_PARENT)

    override fun onAttachedToWindow() {
        super.onAttachedToWindow()

        val surfaceManager = SurfacePresenterManagerImpl(this, logger, runtime.getSurfacePresenterFactory())
        snapDrawingRootHandle.setSurfacePresenterManager(surfaceManager)
    }

    override fun onDetachedFromWindow() {
        super.onDetachedFromWindow()

        snapDrawingRootHandle.setSurfacePresenterManager(null)
    }

    override fun dispatchTouchEvent(event: MotionEvent?): Boolean {
        val handle = this.snapDrawingRootHandle

        if (event == null) {
            return false
        }

        return handle.dispatchTouchEvent(event)
    }

    override fun onMeasure(widthMeasureSpec: Int, heightMeasureSpec: Int) {
        super.onMeasure(widthMeasureSpec, heightMeasureSpec)

        val width = MeasureSpec.getSize(widthMeasureSpec)
        val height = MeasureSpec.getSize(heightMeasureSpec)
        setMeasuredDimension(width, height)

        for (index in 0 until childCount) {
            val child = getChildAt(index)
            child.measure(MeasureSpec.makeMeasureSpec(width, MeasureSpec.EXACTLY),
                    MeasureSpec.makeMeasureSpec(height, MeasureSpec.EXACTLY))
        }
    }

    private fun layoutChild(child: View, width: Int, height: Int) {
        child.layout(0, 0, width , height)
    }

    override fun onLayout(changed: Boolean, left: Int, top: Int, right: Int, bottom: Int) {
        val width = right - left
        val height = bottom - top
        snapDrawingRootHandle.layout(width, height)

        for (index in 0 until childCount) {
            val child = getChildAt(index)
            layoutChild(child, width, height)
        }
    }

    override fun requestLayout() {
        if (ignoreRequestLayoutCount == 0) {
            super.requestLayout()
        }
    }

    private inline fun ignoreRequestLayout(cb: () -> Unit) {
        ignoreRequestLayoutCount++
        try {
            cb()
        } finally {
            ignoreRequestLayoutCount--
        }
    }

    private fun measureAndLayoutPresenterView(presenterView: View, index: Int) {
        val width = this.width
        val height = this.height

        presenterView.measure(MeasureSpec.makeMeasureSpec(width, MeasureSpec.EXACTLY), MeasureSpec.makeMeasureSpec(height, MeasureSpec.EXACTLY))
        layoutChild(presenterView, width, height)
    }

    private fun doRemovePresenterView(presenterView: View) {
        val parent = presenterView.parent ?: return
        if (parent === this) {
            removeViewInLayout(presenterView)
        } else {
            presenterView.removeFromParentView()
        }
    }

    private fun updateDirectChildViewIndex(childView: View, newIndex: Int) {
        detachViewFromParent(childView)
        invalidate()
        attachViewToParent(childView, newIndex, childLayoutParams)
    }

    fun addPresenterViewToZIndex(presenterView: View, zIndex: Int) {
        ignoreRequestLayout {
            if (presenterView.parent === this) {
                // Fast path
                // Removing/adding a view to the hierarchy can be expensive.
                // This will use a faster API which improves performance
                // considerably when moving around text fields.
                updateDirectChildViewIndex(presenterView, zIndex)
            } else {
                // Slow path
                doRemovePresenterView(presenterView)

                invalidate()
                addViewInLayout(presenterView, zIndex, childLayoutParams)

                if (!isLayoutRequested) {
                    measureAndLayoutPresenterView(presenterView, zIndex)
                }
            }
        }
    }

    fun removePresenterView(presenterView: View) {
        ignoreRequestLayout {
            doRemovePresenterView(presenterView)
        }
    }

    private class SurfacePresenterManagerImpl(containerView: SnapDrawingContainerView,
                                              private val logger: Logger,
                                              private val surfacePresenterFactory: SurfacePresenterFactory): SurfacePresenterManager,
            SnapDrawingSurfacePresenterListener {

        val containerView = WeakReference(containerView)
        private var tempRect = Rect()

        private fun getContainerView(): SnapDrawingContainerView? {
            val containerView = this.containerView.get()

            if (containerView == null) {
                logger.error("Failed to retrieve SnapDrawingContainerView")
                return null
            }

            return containerView
        }

        override fun createPresenterWithDrawableSurface(surfacePresenterId: Int, zIndex: Int) {
            val containerView = getContainerView() ?: return

            val presenterView = surfacePresenterFactory.createPresenterWithDrawableSurface(
                    containerView.snapDrawingOptions.renderMode,
                    surfacePresenterId,
                    containerView.snapDrawingOptions.surfaceViewZOrder,
                    this)

            containerView.addPresenterViewToZIndex(presenterView, zIndex)
        }

        override fun createPresenterForEmbeddedView(surfacePresenterId: Int, zIndex: Int, viewToEmbed: ViewRef) {
            val presenterView = surfacePresenterFactory.createPresenterForEmbeddedView(
                    surfacePresenterId,
                    viewToEmbed,
                    this)
            getContainerView()?.addPresenterViewToZIndex(presenterView, zIndex)
        }

        override fun setPresenterZIndex(surfacePresenterId: Int, zIndex: Int) {
            val presenterView = surfacePresenterFactory.getPresenterForId(surfacePresenterId)
            getContainerView()?.addPresenterViewToZIndex(presenterView, zIndex)
        }

        override fun removePresenter(surfacePresenterId: Int) {
            val presenterView = surfacePresenterFactory.getPresenterForId(surfacePresenterId)
            getContainerView()?.removePresenterView(presenterView)
            surfacePresenterFactory.removePresenter(surfacePresenterId)
        }

        override fun setEmbeddedViewPresenterState(surfacePresenterId: Int,
                                                   left: Int,
                                                   top: Int,
                                                   right: Int,
                                                   bottom: Int,
                                                   opacity: Float,
                                                   transform: Any?,
                                                   transformChanged: Boolean,
                                                   clipPath: Any?,
                                                   clipPathChanged: Boolean) {
            surfacePresenterFactory.setEmbeddedViewPresenterState(surfacePresenterId,
                    left,
                    top,
                    right,
                    bottom,
                    opacity,
                    transform,
                    transformChanged,
                    clipPath,
                    clipPathChanged)
        }

        override fun onDrawableSurfacePresenterUpdated(surfacePresenterId: Int) {
            surfacePresenterFactory.onDrawableSurfacePresenterUpdated(surfacePresenterId)
        }

        override fun drawPresenterInCanvas(width: Int,
                                           height: Int,
                                           surfacePresenterId: Int,
                                           canvas: Canvas): Boolean {
            val containerView = getContainerView() ?: return false
            val bitmapHandler = surfacePresenterFactory.bitmapPool.get(width, height) ?: return false

            containerView.snapDrawingRootHandle.drawInBitmap(bitmapHandler, surfacePresenterId, false)

            tempRect.set(0, 0, width, height)
            canvas.drawBitmap(bitmapHandler.getBitmap(), null, tempRect, null)

            containerView.post {
                bitmapHandler.release()
            }

            return true
        }

        override fun onPresenterNeedsRedraw(surfacePresenterId: Int) {
            getContainerView()?.snapDrawingRootHandle?.setSurfaceNeedsRedraw(surfacePresenterId)
        }

        override fun onPresenterHasNewSurface(surfacePresenterId: Int, surface: Surface?) {
            getContainerView()?.snapDrawingRootHandle?.setSurface(surfacePresenterId, surface)
        }

        override fun onPresenterSurfaceSizeChanged(surfacePresenterId: Int) {
            getContainerView()?.snapDrawingRootHandle?.onSurfaceSizeChanged(surfacePresenterId)
        }
    }
}
