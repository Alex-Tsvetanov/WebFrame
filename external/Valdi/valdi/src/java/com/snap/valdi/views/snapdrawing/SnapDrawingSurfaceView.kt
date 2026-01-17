package com.snap.valdi.views.snapdrawing

import android.content.Context
import android.graphics.Canvas
import android.view.SurfaceHolder
import android.view.SurfaceView
import com.snap.valdi.snapdrawing.SnapDrawingSurfacePresenter
import com.snap.valdi.snapdrawing.SnapDrawingSurfacePresenterListener

open class SnapDrawingSurfaceView(context: Context):
        SurfaceView(context),
        SnapDrawingSurfacePresenter,
        SurfaceHolder.Callback,
        SurfaceHolder.Callback2 {

    private var surfacePresenterId = 0
    private var listener: SnapDrawingSurfacePresenterListener? = null

    init {
        setWillNotDraw(false)
    }

    override fun isOpaque(): Boolean {
        return false
    }

    override fun setup(surfacePresenterId: Int, listener: SnapDrawingSurfacePresenterListener?) {
        this.surfacePresenterId = surfacePresenterId
        this.listener = listener

        holder.addCallback(this)
    }

    override fun teardown() {
        this.surfacePresenterId = 0
        this.listener = null

        holder.removeCallback(this)
    }

    override fun onDraw(canvas: Canvas) {
        super.onDraw(canvas)

        val listener = this.listener

        if (listener == null || canvas.isHardwareAccelerated || !isHardwareAccelerated) {
            // Don't draw when rendering on the screen, since the surface will hold the UI
            return
        }

        listener.drawPresenterInCanvas(this.width, this.height, surfacePresenterId, canvas)
    }

    override fun onMeasure(widthMeasureSpec: Int, heightMeasureSpec: Int) {
        super.onMeasure(widthMeasureSpec, heightMeasureSpec)

        setMeasuredDimension(MeasureSpec.getSize(widthMeasureSpec), MeasureSpec.getSize(heightMeasureSpec))
    }

    override fun surfaceRedrawNeeded(holder: SurfaceHolder) {
        listener?.onPresenterNeedsRedraw(surfacePresenterId)
    }

    override fun surfaceChanged(holder: SurfaceHolder, format: Int, width: Int, height: Int) {
        listener?.onPresenterHasNewSurface(surfacePresenterId, holder.surface)
    }

    override fun surfaceDestroyed(holder: SurfaceHolder) {
        listener?.onPresenterHasNewSurface(surfacePresenterId, null)
    }

    override fun surfaceCreated(holder: SurfaceHolder) {
        listener?.onPresenterHasNewSurface(surfacePresenterId, holder.surface)
    }

}
