package com.snap.valdi.views

import com.snap.valdi.utils.CanvasClipper

interface ValdiClippableView {

    val clipper: CanvasClipper
    var clipToBounds: Boolean
    val clipToBoundsDefaultValue: Boolean

}