package com.snap.valdi.drawables

import android.graphics.Canvas
import android.graphics.Rect
import com.snap.valdi.drawables.utils.BorderRadii
import com.snap.valdi.utils.Disposable

interface BoxShadowRenderer: Disposable {

    fun setBoxShadow(widthOffset: Int, heightOffset: Int, blurAmount: Float, color: Int)
    fun draw(canvas: Canvas, bounds: Rect, borderRadii: BorderRadii?)

}