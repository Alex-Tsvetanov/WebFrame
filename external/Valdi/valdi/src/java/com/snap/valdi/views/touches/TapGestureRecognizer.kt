package com.snap.valdi.views.touches

import android.view.MotionEvent
import android.view.View

interface TapGestureRecognizerListener {
    fun onRecognized(gesture: ValdiGestureRecognizer, state: ValdiGestureRecognizerState, x: Int, y: Int, pointerCount: Int, pointerLocations: List<ValdiGesturePointer>)
    fun shouldBegin(gesture: ValdiGestureRecognizer, x: Int, y: Int, pointerCount: Int, pointerLocations: List<ValdiGesturePointer>): Boolean
}

open class TapGestureRecognizer(
        view: View,
        val listener: TapGestureRecognizerListener
) : AndroidDetectorGestureRecognizer(view, false) {

    override fun onSingleTapUp(event: MotionEvent): Boolean {
        updateState(ValdiGestureRecognizerState.BEGAN)
        return true
    }

    override fun onUpdate(event: MotionEvent) {
        if (state == ValdiGestureRecognizerState.POSSIBLE) {
            gestureDetector.onTouchEvent(event)
        }
    }

    override fun onProcess() {
        listener.onRecognized(this, state, x, y, pointerCount, pointerLocations)
    }

    override fun shouldBegin(): Boolean {
        return listener.shouldBegin(this, x, y, pointerCount, pointerLocations)
    }

    override fun requiresFailureOf(other: ValdiGestureRecognizer): Boolean {
        return other is DoubleTapGestureRecognizer || other is AttributedTextTapGestureRecognizer
    }

}
