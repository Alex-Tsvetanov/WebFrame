package com.snap.valdi.attributes.impl.animations

import android.view.View

interface ValdiAnimator {
    fun addValueAnimation(key: Any, view: View, valueAnimation: ValdiValueAnimation?, completion: ((success: Boolean) -> Unit)? = null)

    val beginFromCurrentState: Boolean
}