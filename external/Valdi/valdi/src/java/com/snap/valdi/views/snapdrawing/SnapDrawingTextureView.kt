package com.snap.valdi.views.snapdrawing

import android.content.Context
import android.graphics.SurfaceTexture
import android.view.Surface
import android.view.TextureView
import com.snap.valdi.snapdrawing.SnapDrawingSurfacePresenter
import com.snap.valdi.snapdrawing.SnapDrawingSurfacePresenterListener

class SnapDrawingTextureView(context: Context):
        TextureView(context),
        SnapDrawingSurfacePresenter,
        TextureView.SurfaceTextureListener {

    private var surfacePresenterId: Int = 0
    private var listener: SnapDrawingSurfacePresenterListener? = null
    private var currentSurface: Surface? = null

    init {
        isOpaque = false
    }

    override fun setup(surfacePresenterId: Int, listener: SnapDrawingSurfacePresenterListener?) {
        this.surfacePresenterId = surfacePresenterId
        this.listener = listener

        this.surfaceTextureListener = this
    }

    override fun teardown() {
        this.surfacePresenterId = 0
        this.listener = null
        this.surfaceTextureListener = null

        updateSurface(null)
    }

    fun forceInvalidate() {
        // TextureView doesn't provide a public API to force it to send its texture
        // to the RenderThread. We workaround it by toggling the isOpaque flag twice to
        // cause it to invalidate.
        val isOpaque = this.isOpaque
        setOpaque(!isOpaque)
        setOpaque(isOpaque)
    }

    override fun onMeasure(widthMeasureSpec: Int, heightMeasureSpec: Int) {
        super.onMeasure(widthMeasureSpec, heightMeasureSpec)

        setMeasuredDimension(MeasureSpec.getSize(widthMeasureSpec), MeasureSpec.getSize(heightMeasureSpec))
    }

    private fun updateSurface(newSurface: Surface?) {
        val oldSurface = this.currentSurface
        this.currentSurface = newSurface

        listener?.onPresenterHasNewSurface(surfacePresenterId, newSurface)

        oldSurface?.release()
    }

    override fun onSurfaceTextureAvailable(surface: SurfaceTexture, width: Int, height: Int) {
        updateSurface(createSurface())
    }

    override fun onSurfaceTextureSizeChanged(surface: SurfaceTexture, width: Int, height: Int) {
        listener?.onPresenterSurfaceSizeChanged(surfacePresenterId)
    }

    override fun onSurfaceTextureDestroyed(surface: SurfaceTexture): Boolean {
        updateSurface(null)

        return true
    }

    override fun onSurfaceTextureUpdated(surface: SurfaceTexture) {
    }

    private fun createSurface(): Surface? {
        val texture = this.surfaceTexture ?: return null
        return Surface(texture)
    }
}
