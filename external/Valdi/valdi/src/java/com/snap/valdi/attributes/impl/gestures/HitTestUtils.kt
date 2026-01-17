package com.snap.valdi.attributes.impl.gestures

import android.view.MotionEvent
import android.view.View
import com.snap.valdi.callable.ValdiFunction
import com.snap.valdi.callable.performSync
import com.snap.valdi.utils.ValdiMarshaller
import com.snap.valdi.views.touches.ValdiGesturePointer
import com.snap.valdi.views.touches.ValdiGestureRecognizerState
import kotlin.math.roundToInt

object HitTestUtils {
    fun hitTest(jsHitTest: ValdiFunction, view: View, event: MotionEvent): Boolean {
        jsHitTest ?: return true
        val pointerLocations = ArrayList<ValdiGesturePointer>(event.pointerCount)
        PointerUtils.fillPointerLocations(event, pointerLocations)

        return ValdiMarshaller.use { marshaller ->
            val objectIndex = GestureAttributesUtils.pushGestureParams(
                marshaller,
                view,
                ValdiGestureRecognizerState.POSSIBLE,
                event.x.roundToInt(),
                event.y.roundToInt(),
                event.pointerCount,
                pointerLocations,
                0
            )

            val hasValue = jsHitTest.performSync(marshaller, false)
            if (hasValue) {
                marshaller.getBoolean(-1)
            } else {
                false
            }
        }
    }

}

