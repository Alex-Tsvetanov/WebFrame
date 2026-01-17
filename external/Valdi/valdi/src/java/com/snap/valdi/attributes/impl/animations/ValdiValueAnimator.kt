package com.snap.valdi.attributes.impl.animations

interface ValdiValueAnimator {
    val valueAnimation: ValdiValueAnimation
    fun cancel()
    fun finish()
    fun start()
}