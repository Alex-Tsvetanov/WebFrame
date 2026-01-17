package com.snap.valdi.attributes.impl.animations

import android.animation.ArgbEvaluator
import com.snap.valdi.attributes.impl.animations.ValdiValueAnimation

import androidx.annotation.ColorInt
import android.widget.TextView

class TextAnimator {
    companion object {
        private val argbEvaluator by lazy {
            ArgbEvaluator()
        }

        fun animateTextColor(view: TextView, @ColorInt to: Int): ValdiValueAnimation {
            val startColor = view.currentTextColor
            val valueAnimation = ValdiValueAnimation(ValdiValueAnimation.MinimumVisibleChange.COLOR) { progress ->
                val clampedProgress = progress.coerceIn(0.0f..1.0f)
                val progressColor = argbEvaluator.evaluate(clampedProgress, startColor, to) as Int
                view.setTextColor(progressColor)
            }
            return valueAnimation
        }
    }
}
