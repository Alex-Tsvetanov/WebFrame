package com.snap.valdi.views.touches

import android.view.GestureDetector
import android.view.MotionEvent
import android.view.View
import com.snap.valdi.utils.getValdiHandler

abstract class AndroidDetectorGestureRecognizer(view: View, enableLongPress: Boolean): ValdiGestureRecognizer(view),
        GestureDetector.OnGestureListener,
        GestureDetector.OnDoubleTapListener {

    protected val gestureDetector = GestureDetector(view.context, this, getValdiHandler()).also {
        it.setIsLongpressEnabled(enableLongPress)
    }

    open fun shouldClearGestureDetectorOnReset(): Boolean {
        //This was meant to be overridden to avoid cleanup on FIRST motion up. We do not clean up
        // on first motion up so this should not be needed anymore and we can clean it up once the
        // fix is fully rolled out.
        return true
    }

    override fun onReset(event: MotionEvent) {
        super.onReset(event)

        if (shouldClearGestureDetectorOnReset()) {
            // Clear out the GestureDetector internal state
            val cancelEvent = MotionEvent.obtain(event)
            cancelEvent.action = MotionEvent.ACTION_CANCEL
            gestureDetector.onTouchEvent(cancelEvent)
            cancelEvent.recycle()
        }
    }

    private fun failGesture(): Boolean {
        if (state == ValdiGestureRecognizerState.POSSIBLE) {
            updateState(ValdiGestureRecognizerState.FAILED)
        }

        return false
    }

    override fun onDown(event: MotionEvent): Boolean {
        return true
    }

    override fun onShowPress(event: MotionEvent) {
    }

    override fun onSingleTapUp(event: MotionEvent): Boolean {
        return failGesture()
    }

    override fun onScroll(start: MotionEvent?, end: MotionEvent, distanceX: Float, distanceY: Float): Boolean {
        return failGesture()
    }

    override fun onLongPress(event: MotionEvent) {
        failGesture()
    }

    override fun onFling(start: MotionEvent?, end: MotionEvent, velocityX: Float, velocityY: Float): Boolean {
        return failGesture()
    }

    override fun onSingleTapConfirmed(event: MotionEvent): Boolean {
        return failGesture()
    }

    override fun onDoubleTap(event: MotionEvent): Boolean {
        return failGesture()
    }

    override fun onDoubleTapEvent(event: MotionEvent): Boolean {
        return failGesture()
    }
}