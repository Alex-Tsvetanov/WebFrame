package com.snap.valdi.attributes.impl.animations

import android.animation.TimeInterpolator

import androidx.core.view.animation.PathInterpolatorCompat
import com.snap.valdi.attributes.impl.animations.AnimationType.Companion.ANIMATION_LOOKUP
import com.snap.valdi.exceptions.AttributeError

class ValdiAnimatorFactory {
    companion object {
        @JvmStatic
        fun createAnimation(animationTypeOrdinal: Int, controlPoints: Array<Any>?, duration: Long, beginFromCurrentState: Boolean, springStiffness: Double, springDamping: Double): ValdiAnimator? {
            if (springStiffness > 0 || springDamping > 0) {
                return ValdiSpringAnimator(
                    ValdiSpringConfig(springStiffness, springDamping),
                    beginFromCurrentState
                )
            }

            val interpolator: TimeInterpolator?
            if (controlPoints == null || controlPoints.isEmpty()) {
                interpolator = ANIMATION_LOOKUP[animationTypeOrdinal]?.interpolator
            } else {
                if (controlPoints.size != 4) {
                    throw AttributeError("Wrong number of control points: " + controlPoints.size)
                }

                val cp1 = controlPoints[0] as? Double
                cp1 ?: throw AttributeError("Control point 1 is not a double")

                val cp2 = controlPoints[1] as? Double
                cp2 ?: throw AttributeError("Control point 2 is not a double")

                val cp3 = controlPoints[2] as? Double
                cp3 ?: throw AttributeError("Control point 3 is not a double")

                val cp4 = controlPoints[3] as? Double
                cp4 ?: throw AttributeError("Control point 4 is not a double")

                interpolator = PathInterpolatorCompat.create(
                        cp1.toFloat(),
                        cp2.toFloat(),
                        cp3.toFloat(),
                        cp4.toFloat())
            }

            interpolator ?: return null
            return ValdiInterpolatingAnimator(interpolator, duration, beginFromCurrentState)
        }
    }
}