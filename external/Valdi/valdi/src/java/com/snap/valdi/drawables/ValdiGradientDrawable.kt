package com.snap.valdi.drawables

import android.graphics.*
import android.graphics.drawable.Drawable
import android.graphics.drawable.GradientDrawable
import android.os.Build
import androidx.annotation.ColorInt
import com.snap.valdi.drawables.utils.BorderRadii
import com.snap.valdi.drawables.utils.DrawableInfoProvider
import com.snap.valdi.drawables.utils.FixedDrawableInfoProvider
import kotlin.math.min

/**
 * Just like a normal GradientDrawable, but tracks internal property information for use in mutations
 */
class ValdiGradientDrawable() : Drawable() {

    var strokeWidth: Int = 0
        private set

    @ColorInt
    var strokeColor: Int = Color.TRANSPARENT
        private set

    var gradientColors: IntArray? = null
        private set

    var gradientOffsets: FloatArray? = null
        private set

    var gradientType = GradientDrawable.LINEAR_GRADIENT
        private set

    var gradientOrientation = GradientDrawable.Orientation.TOP_BOTTOM
        private set

    var drawableInfoProvider: DrawableInfoProvider? = null

    private var gradientDirty = false

    @ColorInt
    var fillColor: Int = Color.TRANSPARENT
        private set

    private var mutations: Int = 0

    private val boundsF = RectF()
    private var fillPaint: Paint? = null
    private var strokePaint: Paint? = null
    private var path: Path? = null
    private var boxShadow: BoxShadowRenderer? = null

    fun mutationStart() {
        mutations++
    }

    fun mutationEnd() {
        if (mutations <= 0) {
            throw RuntimeException("Unbalanced mutationStart/mutationEnd calls")
        }

        mutations--
    }

    fun mutationInProgress(): Boolean {
        return mutations > 0
    }

    fun isEmpty(): Boolean {
        if (strokeWidth != 0) {
            return false
        }

        if (strokeColor != Color.TRANSPARENT) {
            return false
        }

        if (boxShadow != null) {
            return false
        }

        if (fillColor != 0) {
            return false
        }

        if (gradientColors != null) {
            return false
        }

        return true
    }

    override fun onBoundsChange(bounds: Rect) {
        super.onBoundsChange(bounds)

        gradientDirty = true
        boundsF.set(bounds)
    }

    fun setBorderRadii(borderRadii: BorderRadii?) {
        drawableInfoProvider = FixedDrawableInfoProvider(borderRadii, null)
        invalidateSelf()
    }

    fun setFillColor(@ColorInt color: Int) {
        this.gradientType = GradientDrawable.LINEAR_GRADIENT
        this.gradientColors = null
        this.gradientOffsets = null
        this.gradientOrientation = GradientDrawable.Orientation.TOP_BOTTOM
        this.fillColor = color

        invalidateSelf()
    }

    fun setFillGradient(type: Int, colors: IntArray?, offsets: FloatArray?, orientation: GradientDrawable.Orientation) {
        this.gradientType = type
        this.gradientColors = colors
        this.gradientOffsets = offsets
        this.gradientOrientation = orientation
        this.fillColor = Color.TRANSPARENT
        this.gradientDirty = true

        invalidateSelf()
    }

    fun setBoxShadow(widthOffset: Int,
                     heightOffset: Int,
                     blurAmount: Float,
                     color: Int,
                     boxShadowRendererPool: BoxShadowRendererPool) {
        if (color == Color.TRANSPARENT) {
            boxShadow?.dispose()
            boxShadow = null
        } else {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
                if (boxShadow == null) {
                    boxShadow = boxShadowRendererPool.create()
                }

                boxShadow?.setBoxShadow(widthOffset, heightOffset, blurAmount, color)
            }
        }
        invalidateSelf()
    }

    fun setStroke(width: Int, @ColorInt color: Int) {
        if (strokeWidth != width || strokeColor != color) {
            strokeWidth = width
            strokeColor = color
            invalidateSelf()
        }
    }

    private fun doDrawPaint(canvas: Canvas, paint: Paint, borderRadii: BorderRadii?) {
        if (borderRadii == null || !borderRadii.hasNonNullValue) {
            // Simplest case, draw a rectangle
            canvas.drawRect(bounds, paint)
        } else {
            val topLeftCornerRadius = borderRadii.getTopLeft(boundsF)
            val topRightCornerRadius = borderRadii.getTopRight(boundsF)
            val bottomRightCornerRadius = borderRadii.getBottomRight(boundsF)
            val bottomLeftCornerRadius = borderRadii.getBottomLeft(boundsF)

            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP &&
                    topLeftCornerRadius == topRightCornerRadius &&
                    topRightCornerRadius == bottomRightCornerRadius &&
                    bottomRightCornerRadius == bottomLeftCornerRadius) {
                // Rounded corners case where all corners are identical
                canvas.drawRoundRect(boundsF.left,
                        boundsF.top,
                        boundsF.right,
                        boundsF.bottom,
                        topLeftCornerRadius,
                        topLeftCornerRadius,
                        paint)
            } else {
                // Complex case, need a path
                var path = this.path
                if (path == null) {
                    path = Path()
                    this.path = path
                } else {
                    path.reset()
                }

                BorderRadii.addToPath(
                        boundsF,
                        topLeftCornerRadius,
                        topRightCornerRadius,
                        bottomRightCornerRadius,
                        bottomLeftCornerRadius,
                        path)

                canvas.drawPath(path, paint)
            }
        }
    }

    private fun updateGradientShader(bounds: RectF, paint: Paint) {
        when (gradientType) {
            GradientDrawable.LINEAR_GRADIENT -> {
                val x0: Float
                val x1: Float
                val y0: Float
                val y1: Float

                when (gradientOrientation) {
                    GradientDrawable.Orientation.TOP_BOTTOM -> {
                        x0 = bounds.left
                        y0 = bounds.top
                        x1 = x0
                        y1 = bounds.bottom
                    }
                    GradientDrawable.Orientation.TR_BL -> {
                        x0 = bounds.right
                        y0 = bounds.top
                        x1 = bounds.left
                        y1 = bounds.bottom
                    }
                    GradientDrawable.Orientation.RIGHT_LEFT -> {
                        x0 = bounds.right
                        y0 = bounds.top
                        x1 = bounds.left
                        y1 = y0
                    }
                    GradientDrawable.Orientation.BR_TL -> {
                        x0 = bounds.right
                        y0 = bounds.bottom
                        x1 = bounds.left
                        y1 = bounds.top
                    }
                    GradientDrawable.Orientation.BOTTOM_TOP -> {
                        x0 = bounds.left
                        y0 = bounds.bottom
                        x1 = x0
                        y1 = bounds.top
                    }
                    GradientDrawable.Orientation.BL_TR -> {
                        x0 = bounds.left
                        y0 = bounds.bottom
                        x1 = bounds.right
                        y1 = bounds.top
                    }
                    GradientDrawable.Orientation.LEFT_RIGHT -> {
                        x0 = bounds.left
                        y0 = bounds.top
                        x1 = bounds.right
                        y1 = y0
                    }
                    GradientDrawable.Orientation.TL_BR -> {
                        x0 = bounds.left
                        y0 = bounds.top
                        x1 = bounds.right
                        y1 = bounds.bottom
                    }
                }

                paint.shader = LinearGradient(x0, y0, x1, y1, gradientColors!!, gradientOffsets, Shader.TileMode.CLAMP)
            }
            GradientDrawable.RADIAL_GRADIENT -> {
                val x0 = bounds.centerX()
                val y0 = bounds.centerY()
                paint.shader  = RadialGradient(x0, y0,
                        min(bounds.width(), bounds.height()) / 2.0f, gradientColors!!, gradientOffsets,
                        Shader.TileMode.CLAMP)
            }
            else -> {
                // Unsupported
                paint.shader = null
            }
        }
    }

    private fun getOrCreateFillPaint(): Paint {
        var fillPaint = this.fillPaint
        if (fillPaint == null) {
            fillPaint = Paint(Paint.ANTI_ALIAS_FLAG)
            fillPaint.style = Paint.Style.FILL
            this.fillPaint = fillPaint
        }

        return fillPaint
    }

    private fun getOrCreateStrokePaint(): Paint {
        var strokePaint = this.strokePaint
        if (strokePaint == null) {
            strokePaint = Paint(Paint.ANTI_ALIAS_FLAG)
            strokePaint.style = Paint.Style.STROKE
            this.strokePaint = strokePaint
        }

        return strokePaint
    }

    private fun doDraw(canvas: Canvas) {
        val borderRadii = this.drawableInfoProvider?.borderRadii

        // Draw shadow first
        boxShadow?.draw(canvas, bounds, borderRadii)

        // Draw background
        if (Color.alpha(fillColor) != 0) {
            // Non transparent fill color

            val fillPaint = getOrCreateFillPaint()
            fillPaint.color = fillColor
            fillPaint.shader = null

            doDrawPaint(canvas, fillPaint, borderRadii)
        } else if (gradientColors != null) {
            // Gradient
            val fillPaint = getOrCreateFillPaint()
            fillPaint.color = Color.BLACK

            if (gradientDirty) {
                gradientDirty = false

                updateGradientShader(boundsF, fillPaint)
            }

            doDrawPaint(canvas, fillPaint, borderRadii)
        }

        // Draw stroke

        if (Color.alpha(strokeColor) != 0 && strokeWidth > 0) {
            val strokePaint = getOrCreateStrokePaint()
            strokePaint.color = strokeColor
            strokePaint.strokeWidth = strokeWidth.toFloat()

            doDrawPaint(canvas, strokePaint, borderRadii)
        }
    }

    override fun draw(canvas: Canvas) {
        val maskPathRenderer = this.drawableInfoProvider?.maskPathRenderer

        if (maskPathRenderer != null && !maskPathRenderer.isEmpty) {
            maskPathRenderer.prepareMask(bounds.width(), bounds.height(), canvas)
            doDraw(canvas)
            maskPathRenderer.applyMask(canvas)
        } else {
            doDraw(canvas)
        }
    }

    override fun setAlpha(alpha: Int) {
        // Unsupported
    }

    override fun setColorFilter(colorFilter: ColorFilter?) {
        // Unsupported
    }

    override fun getOpacity(): Int {
        return PixelFormat.TRANSLUCENT
    }

    companion object {
        fun populateCornerRadiusArray(topLeft: Float,
                                      topRight: Float,
                                      bottomRight: Float,
                                      bottomLeft: Float, cornerRadiusArray: FloatArray) {
            cornerRadiusArray[0] = topLeft
            cornerRadiusArray[1] = topLeft
            cornerRadiusArray[2] = topRight
            cornerRadiusArray[3] = topRight
            cornerRadiusArray[4] = bottomRight
            cornerRadiusArray[5] = bottomRight
            cornerRadiusArray[6] = bottomLeft
            cornerRadiusArray[7] = bottomLeft
        }
    }
}
