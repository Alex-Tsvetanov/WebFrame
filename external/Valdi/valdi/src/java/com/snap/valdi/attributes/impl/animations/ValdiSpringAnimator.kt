package com.snap.valdi.attributes.impl.animations

import android.view.View
import com.snap.valdi.callable.ValdiFunction
import com.snap.valdi.utils.ValdiMarshaller
import kotlin.math.sqrt

import androidx.dynamicanimation.animation.SpringAnimation
import androidx.dynamicanimation.animation.DynamicAnimation
import androidx.dynamicanimation.animation.SpringForce
import androidx.dynamicanimation.animation.FloatValueHolder

private const val TAG = "ValdiSpringAnimator";

data class ValdiSpringConfig(
    val stiffness: Double,
    val damping: Double
)

class ValdiSpringAnimator(
        val springConfig: ValdiSpringConfig,
        override val beginFromCurrentState: Boolean
) : ValdiAnimatorBase<ValdiSpringValueAnimator>() {

    override fun addValueAnimation(key: Any, view: View, valueAnimation: ValdiValueAnimation?, completion: ((success: Boolean) -> Unit)?) {
        valueAnimation ?: return

        val stiffness = springConfig.stiffness
        val damping = springConfig.damping
        val mass = 1
        val dampingRatio = damping / (2*sqrt(stiffness * mass))

        val springForce = SpringForce().also {
            it.stiffness = stiffness.toFloat()
            it.dampingRatio = dampingRatio.toFloat()
        }

        val springValueAnimator = ValdiSpringValueAnimator(key, springForce, valueAnimation)

        if (completion != null) {
            springValueAnimator.springAnimation.addEndListener {  animation, canceled, value, velocity ->
                completion(canceled == false)
            }
        }

        addPendingAnimation(key, view, springValueAnimator)
    }

    override fun flushPendingAnimations(allAnimationsCompletion: () -> Unit) {
        var overallCompletionScheduled = false

        var ongoingCounter = 0
        forEachValidAnimation { pendingAnimation ->
            ongoingCounter += 1
            pendingAnimation.valueAnimator.springAnimation.addEndListener {  animation, canceled, value, velocity ->
                ongoingCounter -= 1

                if (ongoingCounter == 0) {
                    allAnimationsCompletion()
                }
            }
            pendingAnimation.valueAnimator.start()
        }
    }
}
