package com.snap.valdi.attributes.impl.richtext

import com.snap.valdi.views.touches.ValdiGesturePointer
import com.snap.valdi.views.touches.ValdiGestureRecognizer
import com.snap.valdi.views.touches.ValdiGestureRecognizerState
import com.snap.valdi.views.touches.TapGestureRecognizerListener

class OnTapSpan(private val listener: TapGestureRecognizerListener) {

    fun onRecognized(gesture: ValdiGestureRecognizer,
                     state: ValdiGestureRecognizerState,
                     x: Int,
                     y: Int,
                     pointerCount: Int,
                     pointerLocations: List<ValdiGesturePointer>) {
        listener.onRecognized(gesture, state, x, y, pointerCount, pointerLocations)
    }

}