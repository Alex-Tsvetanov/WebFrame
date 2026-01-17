package com.snap.valdi.attributes.impl.animations.transition

import com.snap.valdi.attributes.impl.animations.ValdiValueAnimator

/**
 * Stores information about a View's ongoing value animations.
 */
class ValdiTransitionInfo {

    private val valueAnimators = mutableMapOf<Any, ValdiValueAnimator>()

    // All present value animators apply the final value
    fun finishAllValueAnimators() {
        while (valueAnimators.isNotEmpty()) {
            finishValueAnimator(valueAnimators.keys.first())
        }
    }

    // Does _not_ skip to the end value
    fun cancelValueAnimator(key: Any): Boolean {
        val animator = valueAnimators.remove(key) ?: return false
        animator.cancel()
        return true
    }

    // Skips to the end value
    fun finishValueAnimator(key: Any): Boolean {
        val animator = valueAnimators.remove(key) ?: return false
        animator.finish();
        return true
    }

    fun getValueAnimator(key: Any): ValdiValueAnimator? {
        return valueAnimators[key]
    }

    fun addValueAnimator(key: Any, animator: ValdiValueAnimator) {
        cancelValueAnimator(key)
        valueAnimators[key] = animator
    }
}
