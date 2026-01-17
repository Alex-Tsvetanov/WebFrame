package com.snap.valdi.attributes.impl.animations

import android.animation.Animator
import android.animation.AnimatorListenerAdapter
import android.animation.AnimatorSet
import android.animation.TimeInterpolator
import android.animation.ValueAnimator

import androidx.core.view.animation.PathInterpolatorCompat
import android.view.View
import com.snap.valdi.callable.ValdiFunction
import com.snap.valdi.utils.ValdiMarshaller

class ValdiInterpolatingAnimator(
        val interpolator: TimeInterpolator,
        val duration: Long,
        override val beginFromCurrentState: Boolean
) : ValdiAnimatorBase<ValdiInterpolatingValueAnimator>() {

    private class AnimationCompletion(var completion: ((success: Boolean) -> Unit)?): AnimatorListenerAdapter() {
        private fun notifyCompletion(success: Boolean) {
            val completion = this.completion
            if (completion != null) {
                this.completion = null
                completion(success)
            }
        }

        override fun onAnimationEnd(animation: Animator) {
            notifyCompletion(true)
        }

        override fun onAnimationCancel(animation: Animator) {
            notifyCompletion(false)
        }
    }

    override fun addValueAnimation(key: Any, view: View, valueAnimation: ValdiValueAnimation?, completion: ((success: Boolean) -> Unit)?) {
        valueAnimation ?: return

        val interpolatingValueAnimator = ValdiInterpolatingValueAnimator(valueAnimation)

        if (completion != null) {
            interpolatingValueAnimator.valueAnimator.addListener(AnimationCompletion(completion))
        }

        addPendingAnimation(key, view, interpolatingValueAnimator)
    }

    override fun flushPendingAnimations(allAnimationsCompletion: () -> Unit) {
        val valueAnimators = mutableListOf<Animator>()

        forEachValidAnimation { pendingAnimation ->
            valueAnimators.add(pendingAnimation.valueAnimator.valueAnimator)
        }

        val valueAnimatorSet = AnimatorSet()
        valueAnimatorSet.duration = duration
        valueAnimatorSet.interpolator = interpolator

        valueAnimatorSet.addListener(object: AnimatorListenerAdapter() {
            override fun onAnimationEnd(animation: Animator) {
                allAnimationsCompletion()
            }
        })

        valueAnimatorSet.playTogether(valueAnimators)

        valueAnimatorSet.start()
    }
}
