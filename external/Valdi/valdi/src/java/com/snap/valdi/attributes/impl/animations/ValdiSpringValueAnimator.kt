package com.snap.valdi.attributes.impl.animations

import androidx.dynamicanimation.animation.SpringAnimation
import androidx.dynamicanimation.animation.DynamicAnimation
import androidx.dynamicanimation.animation.SpringForce
import androidx.dynamicanimation.animation.FloatValueHolder

class ValdiSpringValueAnimator(val key: Any, private val springForce: SpringForce, override val valueAnimation: ValdiValueAnimation): ValdiValueAnimator {
    private val floatValueHolder: FloatValueHolder
    val springAnimation: SpringAnimation

    init {
        floatValueHolder = FloatValueHolder(0f)
        springAnimation = SpringAnimation(floatValueHolder)
        springAnimation.spring = springForce
        springAnimation.addUpdateListener { springAnimation, value, velocity ->
            valueAnimation.onProgressUpdate(value)
        }
        springAnimation.minimumVisibleChange = valueAnimation.minimumVisibleChange
    }

    override fun cancel() {
        springAnimation.cancel()
    }

    override fun start() {
        springAnimation.animateToFinalPosition(1f)
    }

    override fun finish() {
        springAnimation.skipToEnd()
    }
}
