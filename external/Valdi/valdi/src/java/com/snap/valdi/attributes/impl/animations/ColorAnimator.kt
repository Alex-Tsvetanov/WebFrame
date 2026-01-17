package com.snap.valdi.attributes.impl.animations

import android.animation.ArgbEvaluator

import com.snap.valdi.drawables.ValdiGradientDrawable

import kotlin.ranges.coerceIn

class ColorAnimator {
    companion object {
        private val argbEvaluator by lazy {
            ArgbEvaluator()
        }

        fun animateGradientDrawable(drawable: ValdiGradientDrawable, toFillColor: Int): ValdiValueAnimation {
            val startingColor = drawable.fillColor

            val valueAnimation =  ValdiValueAnimation(ValdiValueAnimation.MinimumVisibleChange.COLOR) { progress ->
                val clampedProgress = progress.coerceIn(0.0f..1.0f)
                val progressColor = argbEvaluator.evaluate(clampedProgress, startingColor, toFillColor) as Int
                drawable.setFillColor(progressColor)
            }
            return valueAnimation
        }
    }
}