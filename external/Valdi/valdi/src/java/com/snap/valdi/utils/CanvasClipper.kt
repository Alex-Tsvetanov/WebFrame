package com.snap.valdi.utils

import android.graphics.Canvas
import android.graphics.Path
import android.graphics.Rect
import android.graphics.RectF
import com.snap.valdi.drawables.utils.BorderRadii
import com.snap.valdi.drawables.utils.DrawableInfoProvider

class CanvasClipper {

    private var dirty = true

    val hasNonNullCornerRadius: Boolean
        get() = borderRadiiProvider?.borderRadii?.hasNonNullValue ?: false

    var borderRadiiProvider: DrawableInfoProvider? = null

    private val path = Path()
    private val pathRect = RectF()
    private val lastClipBounds = Rect()
    private var lastBorderRadii: BorderRadii? = null

    fun getPath(borderRadii: BorderRadii?, clipBounds: Rect): Path {
        if (lastClipBounds != clipBounds) {
            lastClipBounds.set(clipBounds)
            dirty = true
        }

        if (lastBorderRadii != borderRadii) {
            lastBorderRadii = borderRadii
            dirty = true
        }

        if (dirty) {
            path.reset()
            pathRect.left = clipBounds.left.toFloat()
            pathRect.top = clipBounds.top.toFloat()
            pathRect.right = clipBounds.right.toFloat()
            pathRect.bottom = clipBounds.bottom.toFloat()

            if (borderRadii != null && borderRadii.hasNonNullValue) {
                borderRadii.addToPath(pathRect, path)
            } else {
                path.addRect(pathRect, Path.Direction.CW)
            }
            dirty = false
        }

        return path
    }

    fun getPath(clipBounds: Rect): Path {
        return getPath(borderRadiiProvider?.borderRadii, clipBounds)
    }

    fun clip(canvas: Canvas, clipBounds: Rect) {
        if (hasNonNullCornerRadius) {
            canvas.clipPath(getPath(clipBounds))
        } else {
            canvas.clipRect(clipBounds)
        }
    }

}