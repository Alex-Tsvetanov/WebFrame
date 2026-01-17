package com.snap.valdi.views.snapdrawing

import android.content.Context
import android.graphics.Canvas
import android.graphics.Matrix
import android.graphics.Path
import android.view.View
import android.view.ViewGroup
import com.snap.valdi.extensions.removeFromParentView
import com.snap.valdi.snapdrawing.PathUtils
import com.snap.valdi.snapdrawing.SnapDrawingSurfacePresenter
import com.snap.valdi.snapdrawing.SnapDrawingSurfacePresenterListener

class SnapDrawingEmbeddedView(context: Context): ViewGroup(context), SnapDrawingSurfacePresenter {

    private val clipPath = Path()
    private val transform = Matrix()

    private var viewLeft = 0
    private var viewTop = 0
    private var viewRight = 0
    private var viewBottom = 0
    private var viewAlpha = 1.0f

    var innerView: View?
        get() = if (childCount == 1) getChildAt(0) else null
        set(value) {
            val oldInnerView = this.innerView
            if (oldInnerView !== value) {
                if (oldInnerView != null) {
                    removeViewInLayout(oldInnerView)
                }

                if (value != null) {
                    value.removeFromParentView()
                    addView(value)
                }
            }
        }

    private var surfacePresenterId: Int = 0
    private var hasState = false

    private val isFullySetup: Boolean
        get() = hasState && surfacePresenterId != 0

    override fun setup(surfacePresenterId: Int, listener: SnapDrawingSurfacePresenterListener?) {
        this.surfacePresenterId = surfacePresenterId
    }

    override fun teardown() {
        this.surfacePresenterId = 0
        this.hasState = false
        setState(0,
                0,
                0,
                0,
                1f,
                null,
                true,
                null,
                true)
        innerView = null
    }

    fun setState(left: Int,
                 top: Int,
                 right: Int,
                 bottom: Int,
                 opacity: Float,
                 transformValues: FloatArray?,
                 transformChanged: Boolean,
                 clipPathValues: FloatArray?,
                 clipPathChanged: Boolean) {
        hasState = true
        setStateInner(left, top, right, bottom, opacity, transformValues, transformChanged, clipPathValues, clipPathChanged)
    }

    private fun setStateInner(left: Int,
                 top: Int,
                 right: Int,
                 bottom: Int,
                 opacity: Float,
                 transformValues: FloatArray?,
                 transformChanged: Boolean,
                 clipPathValues: FloatArray?,
                 clipPathChanged: Boolean) {
        if (transformChanged) {
            if (transformValues != null) {
                transform.setValues(transformValues)
            } else {
                transform.reset()
            }
            invalidate()
        }

        viewAlpha = opacity
        innerView?.alpha = opacity

        if (clipPathChanged) {
            clipPath.reset()
            if (clipPathValues != null) {
                PathUtils.parsePath(clipPathValues, clipPath)
            }

            invalidate()
        }

        if (left != viewLeft || top != viewTop || right != viewRight || viewBottom != bottom) {
            viewLeft = left
            viewTop = top
            viewRight = right
            viewBottom = bottom

            if (!isLayoutRequested && isFullySetup) {
                measureInnerView()
                layoutInnerView()
            }
        }
    }

    override fun dispatchDraw(canvas: Canvas) {
        if (canvas == null) {
            return
        }

        if (!clipPath.isEmpty) {
            canvas.clipPath(clipPath)
        }

        canvas.concat(transform)

        super.dispatchDraw(canvas)
    }

    private fun measureInnerView() {
        val innerView = this.innerView ?: return
        val viewWidth = viewRight - viewLeft
        val viewHeight = viewBottom - viewTop

        innerView.measure(
                MeasureSpec.makeMeasureSpec(viewWidth, MeasureSpec.EXACTLY),
                MeasureSpec.makeMeasureSpec(viewHeight, MeasureSpec.EXACTLY)
        )
    }

    private fun layoutInnerView() {
        val innerView = this.innerView ?: return
        innerView.layout(viewLeft, viewTop, viewRight, viewBottom)
    }

    override fun onMeasure(widthMeasureSpec: Int, heightMeasureSpec: Int) {
        val width = MeasureSpec.getSize(widthMeasureSpec)
        val height = MeasureSpec.getSize(heightMeasureSpec)

        setMeasuredDimension(width, height)

        if (isFullySetup) {
            measureInnerView()
        }
    }

    override fun onLayout(changed: Boolean, l: Int, t: Int, r: Int, b: Int) {
        if (isFullySetup) {
            layoutInnerView()
        }
    }

    override fun hasOverlappingRendering(): Boolean {
        return false
    }
}
