package com.snap.valdi.drawables

import android.graphics.Canvas
import android.graphics.Paint
import android.graphics.PorterDuff
import android.graphics.PorterDuffColorFilter
import android.graphics.Rect
import android.os.Build
import androidx.annotation.RequiresApi
import com.snap.valdi.drawables.utils.BorderRadii

class DeferredBoxShadowRenderer(private val pool: BoxShadowRendererPool): BoxShadowRenderer {

    private var widthOffset = 0
    private var heightOffset = 0
    private var blurAmount = 0.0f
    private var color = 0
    private val colorFilterPaint = Paint(Paint.ANTI_ALIAS_FLAG)
    private var previousRenderer: BoxShadowRendererImpl? = null

    override fun setBoxShadow(widthOffset: Int, heightOffset: Int, blurAmount: Float, color: Int) {
        this.widthOffset = widthOffset
        this.heightOffset = heightOffset
        this.blurAmount = blurAmount

        if (this.color != color) {
            this.color = color
            colorFilterPaint.colorFilter = PorterDuffColorFilter(color, PorterDuff.Mode.SRC_IN)
        }
    }

    @RequiresApi(Build.VERSION_CODES.LOLLIPOP)
    private fun getRenderer(bounds: Rect, borderRadii: BorderRadii?): BoxShadowRendererImpl {
        var renderer = this.previousRenderer
        if (renderer == null || renderer.needsConfigure(bounds, borderRadii, widthOffset, heightOffset, blurAmount)) {
            releaseRenderer()
            renderer = pool.createImpl(bounds, borderRadii, widthOffset, heightOffset, blurAmount)
            this.previousRenderer = renderer
        }

        return renderer
    }

    override fun draw(canvas: Canvas, bounds: Rect, borderRadii: BorderRadii?) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
            val renderer = getRenderer(bounds, borderRadii)
            renderer.draw(canvas, colorFilterPaint)
        }
    }

    private fun releaseRenderer() {
        val renderer = this.previousRenderer ?: return
        this.previousRenderer = null
        pool.releaseImpl(renderer)
    }

    override fun dispose() {
        releaseRenderer()
    }
}
