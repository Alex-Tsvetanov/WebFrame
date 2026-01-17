package com.snap.valdi.views

import android.content.Context
import android.graphics.drawable.Drawable

/**
 * A View that supports drawing foreground should implement this interface.
 * The foreground drawable is currently used to render borders on views.
 * If you implement this interface, you should override the dispatchDraw() method and call
 * ViewUtils.drawForeground(this, canvas)
 */
interface ValdiForegroundHolder: Drawable.Callback {
    var valdiForeground: Drawable?

    fun getWidth(): Int
    fun getHeight(): Int
    fun getContext(): Context

    fun invalidate()
}