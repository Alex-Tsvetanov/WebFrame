package com.snap.valdi.attributes.impl.animations

import android.view.View
import com.snap.valdi.attributes.impl.animations.transition.ValdiTransitionInfo
import com.snap.valdi.extensions.ViewUtils
import com.snap.valdi.callable.ValdiFunction
import com.snap.valdi.utils.ValdiMarshaller

abstract class ValdiAnimatorBase<ValueAnimatorType: ValdiValueAnimator> : com.snapchat.client.valdi_core.Animator(), ValdiAnimator {
    private var wasCanceled = false

    protected data class PendingAnimation<ValueAnimatorType: ValdiValueAnimator>(val key: Any, val view: View, val valueAnimator: ValueAnimatorType)

    protected val pendingAnimations = mutableListOf<PendingAnimation<ValueAnimatorType>>()

    protected fun addPendingAnimation(key: Any, view: View, animator: ValueAnimatorType) {
        pendingAnimations.add(PendingAnimation(key, view, animator))

        val transitionInfo = ViewUtils.getOrCreateTransitionInfo(view)
        transitionInfo.addValueAnimator(key, animator)
    }

    protected fun forEachValidAnimation(action: (PendingAnimation<ValueAnimatorType>) -> Unit) {
        for (animation in pendingAnimations) {
            val transitionInfo = ViewUtils.getTransitionInfo(animation.view) ?: continue
            if (!isStillValidAnimation(animation, transitionInfo)) {
                continue
            }
            action(animation)
        }
    }

    protected fun isStillValidAnimation(animation: PendingAnimation<ValueAnimatorType>, transitionInfo: ValdiTransitionInfo): Boolean {
        val valueAnimator = transitionInfo.getValueAnimator(animation.key) ?: return false
        return valueAnimator === animation.valueAnimator
    }

    protected fun removeAnimations() {
        for (animation in pendingAnimations) {
            val transitionInfo = ViewUtils.getTransitionInfo(animation.view) ?: continue
            if (isStillValidAnimation(animation, transitionInfo)) {
                transitionInfo.cancelValueAnimator(animation.key)
            }
        }
        pendingAnimations.clear()
    }

    override fun flushAnimations(target: Any?) {
        flushPendingAnimations {
            removeAnimations()

            val completion = target as? ValdiFunction
            if (completion != null) {
                ValdiMarshaller.use {
                    it.pushBoolean(wasCanceled);
                    completion.perform(it)
                }
            }
        }
    }

    override fun cancel() {
        if (wasCanceled) {
            return
        }

        for (animation in pendingAnimations) {
            val transitionInfo = ViewUtils.getTransitionInfo(animation.view) ?: continue
            if (isStillValidAnimation(animation, transitionInfo)) {
                transitionInfo.finishValueAnimator(animation.key)
            }
        }
        wasCanceled = true
        pendingAnimations.clear()
    }

    abstract open fun flushPendingAnimations(allAnimationsCompletion: () -> Unit)
}
