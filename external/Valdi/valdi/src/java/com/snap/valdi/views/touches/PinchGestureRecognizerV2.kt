package com.snap.valdi.views.touches

import android.view.MotionEvent
import android.view.View

class PinchGestureRecognizerV2 (
        view: View,
        val listener: PinchGestureRecognizerListener
) : ValdiGestureRecognizer(view){

    var scale = 1.0f
    val pinchDetector = PinchGestureDetector(view.context, object : PinchGestureDetector.PinchGestureDetectorListener {
        override fun onScale(scaleFactor: Float) {
            scale = scaleFactor
        }

        override fun onScaleBegin() {
            if (state == ValdiGestureRecognizerState.POSSIBLE) {
                updateState(ValdiGestureRecognizerState.BEGAN)
            }
        }

        override fun onScaleEnd() {
            updateState(ValdiGestureRecognizerState.ENDED)
        }
    })

    override fun shouldBegin(): Boolean {
        return listener.shouldBegin(this, x, y, scale, pointerCount, pointerLocations)
    }

    override fun onUpdate(event: MotionEvent) {
        pinchDetector.onTouchEvent(event)
    }

    override fun onProcess() {
        listener.onRecognized(this, state, x, y, scale, pointerCount, pointerLocations)
    }

    override fun onReset(event: MotionEvent) {
        super.onReset(event)
        scale = 1.0f
    }


    override fun canRecognizeSimultaneously(other: ValdiGestureRecognizer): Boolean {
        return other.javaClass == DragGestureRecognizer::class.java ||
                other.javaClass == RotateGestureRecognizer::class.java ||
                other.javaClass == RotateGestureRecognizerV2::class.java ||
                other.javaClass == ScrollViewDragGestureRecognizer::class.java
    }
}
