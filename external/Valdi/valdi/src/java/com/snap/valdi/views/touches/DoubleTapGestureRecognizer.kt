package com.snap.valdi.views.touches

import android.view.MotionEvent
import android.view.View

interface DoubleTapGestureRecognizerListener {
    fun onRecognized(gesture: DoubleTapGestureRecognizer, state: ValdiGestureRecognizerState, x: Int, y: Int, pointerCount: Int, pointerLocations: List<ValdiGesturePointer>)
    fun shouldBegin(gesture: DoubleTapGestureRecognizer, x: Int, y: Int, pointerCount: Int, pointerLocations: List<ValdiGesturePointer>): Boolean
}

class DoubleTapGestureRecognizer(view: View, val listener: DoubleTapGestureRecognizerListener): AndroidDetectorGestureRecognizer(view, false) {

    override fun shouldBegin(): Boolean {
        return listener.shouldBegin(this, x, y, pointerCount, pointerLocations)
    }

    override fun shouldClearGestureDetectorOnReset(): Boolean {
        // TODO(simon): Workaround for the fact that the TouchDispatcher clears up everything
        // on MOTION UP, which in theory should make double tap impossible to implement.
        // We here explicitly keep the internal state of the gesture detector to allow double tap
        // to work. This should be later changed such that gestures would get an opportunity to
        // stay alive after an UP event in case if another event came through.
        return false
    }

    override fun onUpdate(event: MotionEvent) {
        if (state == ValdiGestureRecognizerState.POSSIBLE) {
            gestureDetector.onTouchEvent(event)
        }
    }

    override fun onProcess() {
        listener.onRecognized(this, state, x, y, pointerCount, pointerLocations)
    }

    override fun onSingleTapUp(event: MotionEvent): Boolean {
        return true
    }

    override fun onDoubleTap(event: MotionEvent): Boolean {
        updateState(ValdiGestureRecognizerState.BEGAN)
        return true
    }

    override fun onDoubleTapEvent(event: MotionEvent): Boolean {
        return true
    }

}