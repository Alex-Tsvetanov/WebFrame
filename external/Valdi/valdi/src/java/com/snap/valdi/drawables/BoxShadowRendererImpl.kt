package com.snap.valdi.drawables

import android.graphics.Bitmap
import android.graphics.BlurMaskFilter
import android.graphics.Canvas
import android.graphics.Color
import android.graphics.Paint
import android.graphics.Path
import android.graphics.Rect
import android.graphics.RectF
import android.os.Build
import androidx.annotation.RequiresApi
import com.snap.valdi.drawables.utils.BorderRadii
import com.snap.valdi.utils.BitmapHandler
import com.snap.valdi.utils.BitmapPool
import kotlin.math.max
import kotlin.math.roundToInt

@RequiresApi(Build.VERSION_CODES.LOLLIPOP)
class BoxShadowRendererImpl(private val bitmapPool: BitmapPool) {

    internal var useCount = 0

    private var shadowPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
        style = Paint.Style.FILL
        color = Color.BLACK
    }

    private var shadowBlurAmount = 0f
    private var shadowWidthOffset = 0
    private var shadowHeightOffset = 0
    private var drawBounds: Rect = Rect()

    private val shadowPath = Path()
    private var shadowPathDirty = true
    private var borderRadii: BorderRadii? = null
    private var adjustedBlurRadius: Float = 0.0f
    private var shadowBitmapHandler: BitmapHandler? = null
    private val renderRect = Rect()
    private val shadowPathRect = RectF()

    private var lastScaledWidth = 0
    private var lastScaledHeight = 0
    private var lastBlurPadding = 0f
    private var needsBitmapRedraw = true

    override fun toString(): String {
        return "[BoxShadowRendererImpl useCount: $useCount, drawBounds: $drawBounds, offset: ${shadowWidthOffset}x${shadowHeightOffset}, blur: $shadowBlurAmount]"
    }

    fun needsConfigure(drawBounds: Rect,
                       borderRadii: BorderRadii?,
                       widthOffset: Int,
                       heightOffset: Int,
                       blurAmount: Float): Boolean {
        if (shadowWidthOffset != widthOffset ||
                shadowHeightOffset != heightOffset ||
                shadowBlurAmount != blurAmount ||
                this.drawBounds != drawBounds ||
                this.borderRadii != borderRadii) {
            return true
        }

        return false
    }

    fun configure(drawBounds: Rect,
                  borderRadii: BorderRadii?,
                  widthOffset: Int,
                  heightOffset: Int,
                  blurAmount: Float) {
        this.drawBounds.set(drawBounds)
        shadowWidthOffset = widthOffset
        shadowHeightOffset = heightOffset
        shadowBlurAmount = blurAmount
        if (this.borderRadii != borderRadii) {
            this.borderRadii = borderRadii
            shadowPathDirty = true
        }
        val boundsWidth = drawBounds.width()
        val boundsHeight = drawBounds.height()

        val downscaleRatio = computeDownscaleRatio(boundsWidth, boundsHeight)

        val scaledWidth = max(boundsWidth / downscaleRatio, 1f).roundToInt()
        val scaledHeight = max(boundsHeight / downscaleRatio, 1f).roundToInt()

        val adjustedBlurRadius = updateBlurRadius(downscaleRatio)

        val padding = computeBlurPadding(adjustedBlurRadius)

        updateShadowPathIfNeeded(scaledWidth, scaledHeight, padding.toFloat(), downscaleRatio)

        val bitmapWidth = scaledWidth + padding * 2
        val bitmapHeight = scaledHeight + padding * 2

        val shadowBitmap = getBitmap(bitmapWidth, bitmapHeight) ?: return

        if (needsBitmapRedraw) {
            needsBitmapRedraw = false
            val bitmapCanvas = Canvas(shadowBitmap)
            bitmapCanvas.drawPath(shadowPath, shadowPaint)
        }

        val upscaledPadding = (padding * downscaleRatio).roundToInt()

        renderRect.set(
                shadowWidthOffset - upscaledPadding,
                shadowHeightOffset - upscaledPadding,
                drawBounds.right + shadowWidthOffset + upscaledPadding,
                drawBounds.bottom + shadowHeightOffset + upscaledPadding
        )
    }

    private fun computeBlurPadding(adjustedBlurRadius: Float): Int {
        // From Skia:
        // https://github.com/servo/skia/blob/master/src/effects/SkBlurMask.cpp#L26
        // https://github.com/servo/skia/blob/master/src/effects/SkBlurMaskFilter.cpp
        val sigmaScale = 0.57735f

        val sigma = if (adjustedBlurRadius > 0) sigmaScale * adjustedBlurRadius + 0.5f else 0.0f
        return Math.round(sigma * 3.0f)
    }

    private fun updateShadowPathIfNeeded(scaledWidth: Int, scaledHeight: Int, blurPadding: Float, downscaleRatio: Float): Path {
        if (scaledWidth != lastScaledWidth || scaledHeight != lastScaledHeight || lastBlurPadding != blurPadding) {
            lastScaledWidth = scaledWidth
            lastScaledHeight = scaledHeight
            lastBlurPadding = blurPadding
            shadowPathDirty = true
        }

        if (shadowPathDirty) {
            shadowPathDirty = false
            shadowPath.reset()

            shadowPathRect.set(blurPadding,
                    blurPadding,
                    scaledWidth.toFloat() + blurPadding,
                    scaledHeight.toFloat() + blurPadding)

            val borderRadii = this.borderRadii
            if (borderRadii != null) {
                borderRadii.addToPathDownscaled(shadowPathRect, shadowPath, downscaleRatio)
            } else {
                shadowPath.addRect(shadowPathRect, Path.Direction.CW)
            }

            needsBitmapRedraw = true
        }
        return shadowPath
    }

    private fun getBitmap(bitmapWidth: Int, bitmapHeight: Int): Bitmap? {
        var bitmap = shadowBitmapHandler?.getBitmap()

        if (bitmap == null || bitmap.width != bitmapWidth || bitmap.height != bitmapHeight) {
            shadowBitmapHandler?.release()
            shadowBitmapHandler = bitmapPool.get(bitmapWidth, bitmapHeight)
            bitmap = shadowBitmapHandler?.getBitmap()
            needsBitmapRedraw = true
        } else {
            if (needsBitmapRedraw) {
                bitmap.eraseColor(Color.TRANSPARENT)
            }
        }

        return bitmap
    }

    private fun updateBlurRadius(downscaleRatio: Float): Float {
        val adjustedBlurAmount = shadowBlurAmount / downscaleRatio * iosToSnapBlurRatio

        if (adjustedBlurRadius != adjustedBlurAmount) {
            adjustedBlurRadius = adjustedBlurAmount

            if (adjustedBlurAmount > 0.0f) {
                shadowPaint.maskFilter = BlurMaskFilter(adjustedBlurAmount, BlurMaskFilter.Blur.NORMAL)
            } else {
                shadowPaint.maskFilter = null
            }
            needsBitmapRedraw = true
        }

        return adjustedBlurRadius
    }

    private fun computeDownscaleRatio(boundsWidth: Int, boundsHeight: Int): Float {
        var downscaleRatio = baseShadowDownscaleRatio

        val maxWidth = bitmapPool.maxWidth
        val maxHeight = bitmapPool.maxHeight
        if (maxWidth > 0 && maxHeight > 0) {
            val adjustedMaxWidth = (maxWidth * downscaleRatio).roundToInt()
            val adjustedMaxHeight = (maxHeight * downscaleRatio).roundToInt()

            if (boundsWidth > adjustedMaxWidth || boundsHeight > adjustedMaxHeight) {
                val xRatio = boundsWidth.toFloat() / adjustedMaxWidth.toFloat()
                val yRatio = boundsHeight.toFloat() / adjustedMaxHeight.toFloat()
                downscaleRatio *= max(xRatio, yRatio)
            }
        }

        return downscaleRatio
    }

    fun draw(canvas: Canvas, paint: Paint) {
        val shadowBitmap = this.shadowBitmapHandler?.getBitmap() ?: return
        canvas.drawBitmap(shadowBitmap, null, renderRect, paint)
    }

    companion object {
        private const val iosToSnapBlurRatio = 2f
        private const val baseShadowDownscaleRatio = 4.0f
    }
}
