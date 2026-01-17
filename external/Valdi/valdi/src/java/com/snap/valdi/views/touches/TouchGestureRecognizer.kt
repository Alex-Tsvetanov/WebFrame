package com.snap.valdi.views.touches

import android.view.MotionEvent
import android.view.View

interface TouchGestureRecognizerListener {
    fun onRecognized(gestureRecognizer: TouchGestureRecognizer, state: ValdiGestureRecognizerState, x: Int, y: Int, pointerCount: Int, pointerLocations: List<ValdiGesturePointer>)
}

open class TouchGestureRecognizer(
    view: View,
    val listener: TouchGestureRecognizerListener
) : ValdiGestureRecognizer(view) {

    override fun canRecognizeSimultaneously(other: ValdiGestureRecognizer): Boolean {
        return true
    }

    override fun shouldBegin(): Boolean {
        return true
    }

    override fun onUpdate(event: MotionEvent) {
        // notify on all touch events, but never capture
        val state = if (newMultiTouchExperience) {
            when (event.actionMasked) {
                MotionEvent.ACTION_DOWN -> ValdiGestureRecognizerState.BEGAN
                MotionEvent.ACTION_POINTER_DOWN ->
                    if (state == ValdiGestureRecognizerState.POSSIBLE)
                        ValdiGestureRecognizerState.BEGAN
                    else
                        ValdiGestureRecognizerState.CHANGED
                MotionEvent.ACTION_UP, MotionEvent.ACTION_CANCEL -> ValdiGestureRecognizerState.ENDED
                else -> ValdiGestureRecognizerState.CHANGED
            }
        } else {
            when (event.actionMasked) {
                MotionEvent.ACTION_DOWN -> ValdiGestureRecognizerState.BEGAN
                MotionEvent.ACTION_UP, MotionEvent.ACTION_CANCEL -> ValdiGestureRecognizerState.ENDED
                else -> ValdiGestureRecognizerState.CHANGED
            }
        }
        updateState(state)
    }

    override fun onProcess() {
        listener.onRecognized(this, state, x, y, pointerCount, pointerLocations)
    }

    override fun shouldPreventInterceptTouchEvent(): Boolean {
        return false
    }

}