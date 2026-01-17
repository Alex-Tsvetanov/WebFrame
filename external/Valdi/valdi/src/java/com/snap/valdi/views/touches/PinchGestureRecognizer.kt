package com.snap.valdi.views.touches

import android.view.MotionEvent
import android.view.View
import android.view.ScaleGestureDetector

interface PinchGestureRecognizerListener {
    fun onRecognized(gesture: ValdiGestureRecognizer, state: ValdiGestureRecognizerState, x: Int, y: Int, scale: Float, pointerCount: Int, pointerLocations: List<ValdiGesturePointer>)
    fun shouldBegin(gesture: ValdiGestureRecognizer, x: Int, y: Int, scale: Float, pointerCount: Int, pointerLocations: List<ValdiGesturePointer>): Boolean
}

/**
 * Deprecated - use PinchGestureRecognizerV2 instead
 */
open class PinchGestureRecognizer(
        view: View,
        val listener: PinchGestureRecognizerListener
) : ValdiGestureRecognizer(view) {

    private var currScale = 1f

    // track previous scales - such that multiple single scales can be all multiplied together
    private var netScale = 1f

    // we split this from pointerCount in ValdiGestureRecognizer to store the curr pointer count of the current event
    private var currPointerCount = 0

    // WARN (mli6) - this may be a cause for gesture start delays for pinch - Ticket: 2854
    private val gestureDetector = ScaleGestureDetector(view.context, object : ScaleGestureDetector.SimpleOnScaleGestureListener() {
        override fun onScaleBegin(detector: ScaleGestureDetector): Boolean {
            if (state == ValdiGestureRecognizerState.POSSIBLE) {
                updateState(ValdiGestureRecognizerState.BEGAN)
            }
            return super.onScaleBegin(detector)
        }

        override fun onScale(detector: ScaleGestureDetector): Boolean {
            currScale = detector.scaleFactor
            return super.onScale(detector)
        }

        override fun onScaleEnd(detector: ScaleGestureDetector) {
            super.onScaleEnd(detector)
            // if any touches are still present, we allow for a scale to continue
            // when end users lift all but one finger, we cache the current scale amount such that they can resume the scale
            if (currPointerCount > 0) {
                netScale *= currScale
                currScale = 1f
            }

            val pointersRequired = 2
            updateState(
                if (pointerCount >= pointersRequired) ValdiGestureRecognizerState.CHANGED
                else ValdiGestureRecognizerState.ENDED
            )
        }
    })

    init {
        gestureDetector.isQuickScaleEnabled = false
    }

    override fun onUpdate(event: MotionEvent) {
        currPointerCount = event.pointerCount

        gestureDetector.onTouchEvent(event)
    }

    override fun onProcess() {
        listener.onRecognized(this, state, x, y, currScale * netScale, pointerCount, pointerLocations)
    }

    override fun shouldBegin(): Boolean {
        return listener.shouldBegin(this, x, y, currScale * netScale, pointerCount, pointerLocations)
    }

    override fun onReset(event: MotionEvent) {
        //Need to ensure ScaleDetector receives the up event so it can reset its state
        gestureDetector.onTouchEvent(event)

        super.onReset(event)
        currPointerCount = 0
        netScale = 1f
        currScale = 1f
    }

    override fun canRecognizeSimultaneously(other: ValdiGestureRecognizer): Boolean {
        return other.javaClass == DragGestureRecognizer::class.java ||
                other.javaClass == RotateGestureRecognizer::class.java ||
                other.javaClass == RotateGestureRecognizerV2::class.java
    }
}
