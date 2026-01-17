package com.snap.valdi.views.touches

import android.util.Log
import android.view.GestureDetector
import android.view.MotionEvent
import android.view.View
import android.view.ScaleGestureDetector
import com.snap.valdi.attributes.impl.gestures.RotationTracker

interface RotateGestureRecognizerListener {
    fun onRecognized(gesture: ValdiGestureRecognizer, state: ValdiGestureRecognizerState, x: Int, y: Int, rotation: Float, pointerCount: Int, pointerLocations: List<ValdiGesturePointer>)
    fun shouldBegin(gesture: ValdiGestureRecognizer, x: Int, y: Int, rotation: Float, pointerCount: Int, pointerLocations: List<ValdiGesturePointer>): Boolean
}

/**
 * Deprecated - use RotateGestureRecognizerV2 instead. This will become the default behavior
 * after SNAP_EDITOR_ROTATION_V2_ENABLED has been validated.
 */
open class RotateGestureRecognizer(
        view: View,
        val listener: RotateGestureRecognizerListener
) : ValdiGestureRecognizer(view) {

    private val rotationTracker = RotationTracker()
    private var currRotation = 0f

    // we split this from pointerCount in ValdiGestureRecognizer to store the curr pointer count of the current event
    private var currPointerCount = 0

    // track previous rotations - such that multiple single rotations can be all added together
    private var netRotation = 0f

    // WARN (mli6) - this may be a cause for gesture start delays for rotate - 2854
    private val gestureDetector = ScaleGestureDetector(view.context, object : ScaleGestureDetector.SimpleOnScaleGestureListener() {
        override fun onScaleBegin(detector: ScaleGestureDetector): Boolean {
            if (state == ValdiGestureRecognizerState.POSSIBLE) {
                updateState(ValdiGestureRecognizerState.BEGAN)
            }
            return super.onScaleBegin(detector)
        }

        override fun onScaleEnd(detector: ScaleGestureDetector) {
            super.onScaleEnd(detector)
            Log.d("${this::class.simpleName}","$viewIdentifier onScaleEnd pointerCount $pointerCount")

            // if any touches are still present, we allow for a rotation to continue
            // when end users lift all but one finger, we cache the current rotation amount such that they can resume the rotation
            if (currPointerCount > 0) {
                netRotation += currRotation
                currRotation = 0f
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
        rotationTracker.onTouchEvent(event)
        currRotation = rotationTracker.rotation
    }

    override fun onProcess() {
        listener.onRecognized(this, state, x, y, currRotation + netRotation, pointerCount, pointerLocations)
    }

    override fun shouldBegin(): Boolean {
        return listener.shouldBegin(this, x, y, currRotation + netRotation, pointerCount, pointerLocations)
    }

    override fun onReset(event: MotionEvent) {
        //Need to ensure ScaleDetector receives the up event so it can reset its state
        gestureDetector.onTouchEvent(event)
        super.onReset(event)
        currPointerCount = 0
        currRotation = 0f
        netRotation = 0f
    }

    override fun canRecognizeSimultaneously(other: ValdiGestureRecognizer): Boolean {
        return other.javaClass == DragGestureRecognizer::class.java ||
            other.javaClass == PinchGestureRecognizer::class.java
    }
}
