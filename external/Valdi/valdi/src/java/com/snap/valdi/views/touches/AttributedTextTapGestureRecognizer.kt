package com.snap.valdi.views.touches

import android.text.Spannable
import android.view.MotionEvent
import android.widget.TextView
import com.snap.valdi.attributes.impl.richtext.OnTapSpan

class AttributedTextTapGestureRecognizer(view: TextView):
        AndroidDetectorGestureRecognizer(view, false) {

    var spannable: Spannable? = null

    private var tappedSpan: OnTapSpan? = null

    override fun onSingleTapUp(event: MotionEvent): Boolean {
        if (processTap(event)) {
            updateState(ValdiGestureRecognizerState.BEGAN)
        } else {
            updateState(ValdiGestureRecognizerState.FAILED)
        }
        return true
    }

    override fun shouldBegin(): Boolean {
        return true
    }

    override fun onUpdate(event: MotionEvent) {
        if (state == ValdiGestureRecognizerState.POSSIBLE) {
            gestureDetector.onTouchEvent(event)
        }
    }

    override fun onProcess() {
        tappedSpan?.onRecognized(this, state, x, y, pointerCount, pointerLocations)
    }

    override fun onReset(event: MotionEvent) {
        super.onReset(event)

        tappedSpan = null
    }

    private fun processTap(event: MotionEvent): Boolean {
        this.tappedSpan = null

        val textView = this.view as TextView
        val spannable = this.spannable ?: return false

        val offset = textView.getOffsetForPosition(event.x, event.y)
        if (offset < 0 || offset >= spannable.length) {
            return false
        }

        val spans = spannable.getSpans(offset, offset, OnTapSpan::class.java)
        if (spans.isNullOrEmpty()) {
            return false
        }

        this.tappedSpan = spans.first()

        return true
    }

    override fun requiresFailureOf(other: ValdiGestureRecognizer): Boolean {
        return other is DoubleTapGestureRecognizer
    }

}