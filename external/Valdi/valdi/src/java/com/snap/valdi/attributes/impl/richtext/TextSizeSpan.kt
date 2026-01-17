package com.snap.valdi.attributes.impl.richtext

import android.text.TextPaint
import android.text.style.MetricAffectingSpan

class TextSizeSpan(private val fontSize: Float): MetricAffectingSpan() {
    private fun updateTextPaint(textPaint: TextPaint) {
        textPaint.textSize = fontSize
    }

    override fun updateDrawState(tp: TextPaint) {
        updateTextPaint(tp)
    }

    override fun updateMeasureState(textPaint: TextPaint) {
        updateTextPaint(textPaint)
    }
}
