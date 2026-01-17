package com.snap.valdi.attributes.impl.animations

import android.animation.ValueAnimator

class ValdiInterpolatingValueAnimator(override val valueAnimation: ValdiValueAnimation): ValdiValueAnimator {
    val valueAnimator: ValueAnimator

    init {
        valueAnimator = ValueAnimator.ofFloat(0f, 1f)
        valueAnimator.addUpdateListener {
            valueAnimation.onProgressUpdate(it.animatedValue as Float)
        }
    }

    override fun cancel() {
        valueAnimator.cancel()
    }

    override fun start() {
        valueAnimator.start()
    }

    override fun finish() {
        valueAnimator.end()
    }
}