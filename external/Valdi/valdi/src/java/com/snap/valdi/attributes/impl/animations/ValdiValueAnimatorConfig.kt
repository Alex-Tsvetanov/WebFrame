package com.snap.valdi.attributes.impl.animations

class ValdiValueAnimation(
    // Used to optimize dynamic animations which will take this value into account
    // when calculating how the animation "settles" (e.g. spring animations).
    val minimumVisibleChange: Float,
    // You can associate some data with this animation.
    val additionalData: Any? = null,
    // This block gets called on every "tick" of the animation with the 'progress'
    // representing the progress of the animation from 0.0 to 1.0.
    val onProgressUpdate: (progress: Float) -> Unit
) {
    class MinimumVisibleChange {
        companion object {
            const val COLOR = 0.00390f

            const val ALPHA = 0.00390f

            // Could be optimized further by taking into account runtime display density, this value just assumed max @3x
            const val PIXEL = 0.00016f

            // assuming 0-6 is a reasonable scale range
            const val SCALE_RATIO = 0.0004f

            // assuming 0-2*pi
            const val ROTATION_DEGREES_ANGLE = 0.00066f
        }
    }
}