package com.snap.valdi.attributes.impl.gestures

import android.view.View
import com.snap.valdi.callable.ValdiFunction
import com.snap.valdi.callable.performThrottled
import com.snap.valdi.extensions.ViewUtils
import com.snap.valdi.nodes.ValdiViewNode
import com.snap.valdi.utils.ValdiMarshaller
import com.snap.valdi.utils.EventTime
import com.snap.valdi.utils.InternedString
import com.snap.valdi.views.touches.ValdiGesturePointer
import com.snap.valdi.views.touches.ValdiGestureRecognizerState

object GestureAttributesUtils {

    /**
     * Create an object into the ValdiMarshaller, filling it with common gesture properties
     */
    fun pushGestureParams(
        marshaller: ValdiMarshaller,
        view: View,
        state: ValdiGestureRecognizerState,
        x: Int,
        y: Int,
        pointerCount: Int,
        pointerLocations: List<ValdiGesturePointer>,
        additionalParamsCount: Int = 0
    ): Int {
        val objectIndex = marshaller.pushMap(8 + additionalParamsCount)

        val pixelDensity = view.context.resources.displayMetrics.density.toDouble()

        if (pixelDensity <= 0) {
            return objectIndex
        }

        val valdiState = when (state) {
            ValdiGestureRecognizerState.BEGAN -> 0
            ValdiGestureRecognizerState.CHANGED -> 1
            ValdiGestureRecognizerState.ENDED -> 2
            ValdiGestureRecognizerState.POSSIBLE -> 3
            else -> return objectIndex
        }

        var relativeX = x
        var relativeY = y
        var absoluteX = x
        var absoluteY = y

        val viewNode = ViewUtils.findViewNode(view)

        if (viewNode != null) {
            val relativePosition = viewNode.convertPointToRelativeDirectionAgnostic(x, y)
            val absolutePosition = viewNode.convertPointToAbsoluteDirectionAgnostic(x, y)

            relativeX = ValdiViewNode.horizontalFromEncodedLong(relativePosition)
            relativeY = ValdiViewNode.verticalFromEncodedLong(relativePosition)
            absoluteX = ValdiViewNode.horizontalFromEncodedLong(absolutePosition)
            absoluteY = ValdiViewNode.verticalFromEncodedLong(absolutePosition)
        }

        val eventTime = EventTime.getTimeSeconds()

        marshaller.putMapPropertyDouble(eventTimeProperty, objectIndex, eventTime)
        marshaller.putMapPropertyDouble(stateProperty, objectIndex, valdiState.toDouble())
        marshaller.putMapPropertyDouble(xProperty, objectIndex, relativeX / pixelDensity)
        marshaller.putMapPropertyDouble(yProperty, objectIndex, relativeY / pixelDensity)
        marshaller.putMapPropertyDouble(absoluteXProperty, objectIndex, absoluteX / pixelDensity)
        marshaller.putMapPropertyDouble(absoluteYProperty, objectIndex, absoluteY / pixelDensity)
        marshaller.putMapPropertyInt(pointerCountProperty, objectIndex, pointerCount)
        marshaller.putMapPropertyList(pointerLocationsProperty, objectIndex, pointerLocations) {
            val listItemIndex = marshaller.pushMap(2)
            marshaller.putMapPropertyDouble(xProperty, listItemIndex, it.x / pixelDensity)
            marshaller.putMapPropertyDouble(yProperty, listItemIndex, it.y / pixelDensity)
            listItemIndex
        }

        return objectIndex
    }

    private val stateProperty = InternedString.create("state")
    private val xProperty = InternedString.create("x")
    private val yProperty = InternedString.create("y")
    private val absoluteXProperty = InternedString.create("absoluteX")
    private val absoluteYProperty = InternedString.create("absoluteY")
    private val eventTimeProperty = InternedString.create("eventTime")
    private val pointerCountProperty = InternedString.create("pointerCount")
    private val pointerLocationsProperty = InternedString.create("pointerLocations")

    fun performGestureCallback(function: ValdiFunction?, state: ValdiGestureRecognizerState, marshaller: ValdiMarshaller) {
        if (function == null) {
            return
        }

        if (state == ValdiGestureRecognizerState.CHANGED) {
            function.performThrottled(marshaller)
        } else {
            function.perform(marshaller)
        }
    }

}
