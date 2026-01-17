package com.snap.valdi.attributes.impl.richtext

import android.graphics.Canvas
import android.graphics.Paint
import android.text.style.ReplacementSpan
import kotlin.math.roundToInt


class OutlineReplacementSpan(
    private val color: Int,
    private val outlineColor: Int,
    private val outlineWidth: Float
): ReplacementSpan() {
    override fun getSize(
        paint: Paint,
        text: CharSequence?,
        start: Int,
        end: Int,
        fm: Paint.FontMetricsInt?
    ): Int {
        // explicitly set size we define earlier
        if (fm != null) {
            val metrics = paint.fontMetricsInt
            fm.ascent = metrics.ascent
            fm.descent = metrics.descent
            fm.top = metrics.top
            fm.bottom = metrics.bottom
        }

        return paint.measureText(text, start, end).roundToInt()
    }

    override fun draw(
        canvas: Canvas,
        text: CharSequence?,
        start: Int,
        end: Int,
        x: Float,
        top: Int,
        y: Int,
        bottom: Int,
        paint: Paint
    ) {
        paint.style = Paint.Style.STROKE
        paint.strokeWidth = outlineWidth
        paint.color = outlineColor
        canvas.drawText(text!!, start, end, x, y.toFloat(), paint)

        paint.style = Paint.Style.FILL
        paint.color = color
        canvas.drawText(text, start, end, x, y.toFloat(), paint)
    }
}

