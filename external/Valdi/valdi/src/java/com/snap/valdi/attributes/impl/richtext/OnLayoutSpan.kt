package com.snap.valdi.attributes.impl.richtext

import com.snap.valdi.callable.ValdiFunction
import com.snap.valdi.utils.ValdiMarshaller
import com.snap.valdi.utils.InternedString
import com.snap.valdi.views.touches.ValdiGestureRecognizer
import com.snap.valdi.views.touches.ValdiGestureRecognizerState
import com.snap.valdi.views.touches.TapGestureRecognizerListener

class OnLayoutSpan(private val function: ValdiFunction, val start: Int, val length: Int) {

    fun onLayout(x: Double, y: Double, width: Double, height: Double) {
        ValdiMarshaller.use {
            it.pushDouble(x)
            it.pushDouble(y)
            it.pushDouble(width)
            it.pushDouble(height)
            function.perform(it)
        }
    }
}
