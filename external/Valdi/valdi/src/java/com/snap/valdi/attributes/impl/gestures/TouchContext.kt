package com.snap.valdi.attributes.impl.gestures

import com.snap.valdi.callable.ValdiFunction
import com.snap.valdi.utils.ValdiMarshaller
import com.snap.valdi.views.touches.ValdiGesturePointer
import com.snap.valdi.views.touches.ValdiGestureRecognizerState
import com.snap.valdi.views.touches.TouchGestureRecognizer
import com.snap.valdi.views.touches.TouchGestureRecognizerListener

class TouchContext: TouchGestureRecognizerListener {

    var action: ValdiFunction? = null
    var startAction: ValdiFunction? = null
    var endAction: ValdiFunction? = null

    override fun onRecognized(gestureRecognizer: TouchGestureRecognizer, state: ValdiGestureRecognizerState, x: Int, y: Int, pointerCount: Int, pointerLocations: List<ValdiGesturePointer>) {
        ValdiMarshaller.use {
            GestureAttributesUtils.pushGestureParams(it, gestureRecognizer.view, state, x, y, pointerCount, pointerLocations)

            if (state == ValdiGestureRecognizerState.BEGAN) {
                startAction?.perform(it)
            } else if (state == ValdiGestureRecognizerState.ENDED) {
                endAction?.perform(it)
            }

            GestureAttributesUtils.performGestureCallback(action, state, it)
        }
    }

}