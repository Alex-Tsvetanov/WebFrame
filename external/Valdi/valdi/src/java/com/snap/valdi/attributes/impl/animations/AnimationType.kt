package com.snap.valdi.attributes.impl.animations

import android.animation.TimeInterpolator
import android.view.animation.AccelerateDecelerateInterpolator
import android.view.animation.AccelerateInterpolator
import android.view.animation.DecelerateInterpolator
import android.view.animation.LinearInterpolator

// Ordinals of this enum correspond to Valdi animations, so don't rearrange them
enum class AnimationType(val interpolator: TimeInterpolator) {
    LINEAR(LinearInterpolator()),
    EASE_IN(AccelerateInterpolator()),
    EASE_OUT(DecelerateInterpolator()),
    EASE_IN_OUT(AccelerateDecelerateInterpolator());

    companion object {
        val ANIMATION_LOOKUP = mapOf(*AnimationType.values().map { Pair(it.ordinal, it) }.toTypedArray())
    }
}