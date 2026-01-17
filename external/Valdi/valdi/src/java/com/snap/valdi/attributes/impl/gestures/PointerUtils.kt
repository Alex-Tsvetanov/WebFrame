package com.snap.valdi.attributes.impl.gestures

import android.view.MotionEvent
import com.snap.valdi.views.touches.ValdiGesturePointer
import kotlin.math.roundToInt

object PointerUtils {
    fun fillPointerLocations(event: MotionEvent, pointerLocations: MutableList<ValdiGesturePointer>) {
        for (index in 0 until event.pointerCount) {
            val x = event.getX(index).roundToInt()
            val y = event.getY(index).roundToInt()
            val pointerId = event.getPointerId(index)
            pointerLocations.add(ValdiGesturePointer(x, y, pointerId))
        }
    }
}