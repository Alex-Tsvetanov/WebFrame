package com.snap.valdi.views

import androidx.annotation.Keep
import android.content.Context
import android.graphics.Canvas
import android.graphics.Color
import android.graphics.Paint
import android.graphics.Path
import android.view.View
import com.snap.valdi.extensions.ViewUtils
import com.snap.valdi.utils.CoordinateResolver
import com.snap.valdi.utils.GeometricPath
import com.snap.valdi.utils.PathInterpolator

private val DEFAULT_STROKE_WIDTH = 1.0f
private val DEFAULT_COLOR = Color.TRANSPARENT

@Keep
class ShapeView(context: Context) : View(context), ValdiRecyclableView {

    companion object {
        private const val TAG = "ShapeView"
    }

    var strokeStart: Float = 0.0f
        set(value) {
            if (field != value) {
                field = value
                invalidate()
            }
        }

    var strokeEnd: Float = 1.0f
        set(value) {
            if (field != value) {
                field = value
                invalidate()
            }
        }

    private val geometricPath = GeometricPath()
    private val strokePaint = Paint()
    private val fillPaint = Paint()
    private val coordinateResolver = CoordinateResolver(context)
    private var pathInterpolator: PathInterpolator? = null

    init {
        strokePaint.strokeJoin = Paint.Join.MITER
        strokePaint.strokeCap = Paint.Cap.BUTT
        strokePaint.style = Paint.Style.STROKE

        fillPaint.style = Paint.Style.FILL

        strokePaint.isAntiAlias = true
        fillPaint.isAntiAlias = true

        resetStrokeColor()
        resetFillColor()
        resetStrokeWidth()
        resetStrokeCap()
        resetStrokeJoin()
    }

    fun setPathData(pathData: ByteArray?) {
        geometricPath.setPathData(pathData)
        pathInterpolator?.reset()
        invalidate()
    }

    fun setStrokeWidth(strokeWidth: Float) {
        strokePaint.strokeWidth = coordinateResolver.toPixelF(strokeWidth)
        invalidate()
    }

    fun resetStrokeWidth() {
        setStrokeWidth(DEFAULT_STROKE_WIDTH)
    }

    fun setStrokeColor(strokeColor: Int) {
        strokePaint.color = strokeColor
        invalidate()
    }

    fun resetStrokeColor() {
        setStrokeColor(DEFAULT_COLOR)
    }

    fun setFillColor(fillColor: Int) {
        fillPaint.color = fillColor
        invalidate()
    }

    fun resetFillColor() {
        setFillColor(DEFAULT_COLOR)
    }

    fun setStrokeJoin(strokeJoin: Paint.Join) {
        strokePaint.strokeJoin = strokeJoin
        invalidate()
    }

    fun resetStrokeJoin() {
        setStrokeJoin(Paint.Join.MITER)
    }

    fun setStrokeCap(strokeCap: Paint.Cap) {
        strokePaint.strokeCap = strokeCap
        invalidate()
    }

    fun resetStrokeCap() {
        setStrokeCap(Paint.Cap.BUTT)
    }

    override fun onDraw(canvas: Canvas) {
        ViewUtils.onDraw(this, canvas) {
            super.onDraw(it)

            if (geometricPath.isEmpty) {
                return
            }

            val activePath = this.getActivePath()

            it.drawPath(activePath, fillPaint)
            it.drawPath(activePath, strokePaint)
        }
    }

    private fun getActivePath(): Path {
        geometricPath.width = width
        geometricPath.height = height
        val path = geometricPath.path
        if (strokeStart == 0.0f && strokeEnd == 1.0f) {
            return path
        }

        var pathInterpolator = this.pathInterpolator
        if (pathInterpolator == null) {
            pathInterpolator = PathInterpolator()
            this.pathInterpolator = pathInterpolator
        }

        if (pathInterpolator.empty) {
            pathInterpolator.setPath(path)
        }

        return pathInterpolator.interpolate(strokeStart, strokeEnd)
    }

    override fun hasOverlappingRendering(): Boolean {
        return ViewUtils.hasOverlappingRendering(this)
    }

}