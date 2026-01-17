package com.snap.valdi.attributes.impl.animations

import android.animation.ArgbEvaluator
import android.graphics.RectF

import androidx.annotation.ColorInt
import androidx.annotation.RequiresApi
import android.view.View
import com.snap.valdi.drawables.ValdiGradientDrawable
import com.snap.valdi.drawables.utils.BorderRadii
import com.snap.valdi.extensions.ViewUtils
import com.snap.valdi.utils.CoordinateResolver

class ViewAnimator {
    companion object {
        private val argbEvaluator by lazy {
            ArgbEvaluator()
        }

        fun animateAlpha(view: View, alpha: Float): ValdiValueAnimation {
            val startAlpha = view.alpha
            val valueAnimation = ValdiValueAnimation(ValdiValueAnimation.MinimumVisibleChange.ALPHA) { progress ->
                val clampedProgress = progress.coerceIn(0.0f..1.0f)
                val resultValue = interpolate(clampedProgress, startAlpha, alpha)
                view.alpha = resultValue
            }
            return valueAnimation
        }

        fun interpolate(progress: Float, from: Float, to: Float): Float {
            return (to - from) * progress + from
        }

        fun animateBorder(view: View, drawable: ValdiGradientDrawable, borderWidth: Int, @ColorInt color: Int): ValdiValueAnimation {
            val startColor = drawable.strokeColor
            val startWidth = drawable.strokeWidth
            val valueAnimation = ValdiValueAnimation(ValdiValueAnimation.MinimumVisibleChange.PIXEL) { progress ->
                val progressWidth = interpolate(progress, startWidth.toFloat(), borderWidth.toFloat()).toInt()
                val progressColor = argbEvaluator.evaluate(progress, startColor, color) as Int
                drawable.setStroke(progressWidth, progressColor)
                view.invalidate()
            }
            return valueAnimation
        }

        fun animateBorderRadius(view: View, borderRadii: BorderRadii?): ValdiValueAnimation {
            val fromBorderRadii = ViewUtils.getBorderRadii(view)
            val viewRect = RectF()

            // Can probably optimize minimumVisibleChange by considering that the range of values is limited by the view width or height (divided by 2)
            val valueAnimation = ValdiValueAnimation(ValdiValueAnimation.MinimumVisibleChange.PIXEL, additionalData=borderRadii) { progress ->
                viewRect.set(view.left.toFloat(), view.top.toFloat(), view.right.toFloat(), view.bottom.toFloat())

                val topLeftStart = fromBorderRadii?.getTopLeft(viewRect) ?: 0.0f
                val topRightStart = fromBorderRadii?.getTopRight(viewRect) ?: 0.0f
                val bottomRightStart = fromBorderRadii?.getBottomRight(viewRect) ?: 0.0f
                val bottomLeftStart = fromBorderRadii?.getBottomLeft(viewRect) ?: 0.0f

                val topLeftRadius = borderRadii?.getTopLeft(viewRect) ?: 0.0f
                val topRightRadius = borderRadii?.getTopRight(viewRect) ?: 0.0f
                val bottomRightRadius = borderRadii?.getBottomRight(viewRect) ?: 0.0f
                val bottomLeftRadius = borderRadii?.getBottomLeft(viewRect) ?: 0.0f

                val currentTopLeft = interpolate(progress, topLeftStart, topLeftRadius)
                val currentTopRight = interpolate(progress, topRightStart, topRightRadius)
                val currentBottomRight = interpolate(progress, bottomRightStart, bottomRightRadius)
                val currentBottomLeft = interpolate(progress, bottomLeftStart, bottomLeftRadius)

                ViewUtils.setBorderRadii(view, BorderRadii(currentTopLeft, currentTopRight, currentBottomRight, currentBottomLeft, false, false, false, false))
            }

            return valueAnimation
        }

        inline fun animateTransformElement(value: Float, view: View, crossinline getValue: View.() -> Float, crossinline setValue: View.(Float) -> Unit, minimumVisibleChange: Float): ValdiValueAnimation {
            val startValue = view.getValue()
            val animator = ValdiValueAnimation(minimumVisibleChange=minimumVisibleChange, additionalData = value) { progress ->
                val resultValue = interpolate(progress, startValue, value)
                view.setValue(resultValue)
            }

            return animator
        }

        @RequiresApi(21)
        fun animateElevation(view: View, targetElevation: Float): ValdiValueAnimation {
            val startElevation = view.elevation
            val valueAnimation = ValdiValueAnimation(minimumVisibleChange=ValdiValueAnimation.MinimumVisibleChange.PIXEL) { progress -> 
                val resultValue = interpolate(progress, startElevation, targetElevation)
                view.elevation = resultValue
            }
            return valueAnimation
        }
    }
}
