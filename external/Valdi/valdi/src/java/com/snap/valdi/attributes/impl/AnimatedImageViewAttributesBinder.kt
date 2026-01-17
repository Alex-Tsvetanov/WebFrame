package com.snap.valdi.attributes.impl

import android.content.Context
import android.widget.ImageView
import com.snap.valdi.attributes.AttributesBinder
import com.snap.valdi.attributes.AttributesBindingContext
import com.snap.valdi.attributes.impl.animations.ValdiAnimator
import com.snap.valdi.callable.ValdiFunction
import com.snap.valdi.exceptions.AttributeError
import com.snap.valdi.views.AnimatedImageView
import com.snapchat.client.valdi_core.AssetOutputType

class AnimatedImageViewAttributesBinder(private val context: Context): AttributesBinder<AnimatedImageView> {

    override val viewClass: Class<AnimatedImageView>
        get() = AnimatedImageView::class.java

    private fun applyLoop(view: AnimatedImageView, value: Boolean, animator: ValdiAnimator?) {
        view.setShouldLoop(value)
    }

    private fun resetLoop(view: AnimatedImageView, animator: ValdiAnimator?) {
        view.setShouldLoop(false)
    }

    private fun applyAdvanceRate(view: AnimatedImageView, value: Float, animator: ValdiAnimator?) {
        view.setAdvanceRate(value.toDouble())
    }

    private fun resetAdvanceRate(view: AnimatedImageView, animator: ValdiAnimator?) {
        view.setAdvanceRate(0.0)
    }

    private fun applyCurrentTime(view: AnimatedImageView, value: Float, animator: ValdiAnimator?) {
        view.setCurrentTime(value.toDouble())
    }

    private fun resetCurrentTime(view: AnimatedImageView, animator: ValdiAnimator?) {
        view.setCurrentTime(0.0)
    }

    private fun applyAnimationStartTime(view: AnimatedImageView, value: Float, animator: ValdiAnimator?) {
        view.setAnimationStartTime(value.toDouble())
    }

    private fun resetAnimationStartTime(view: AnimatedImageView, animator: ValdiAnimator?) {
        view.setAnimationStartTime(0.0)
    }

    private fun applyAnimationEndTime(view: AnimatedImageView, value: Float, animator: ValdiAnimator?) {
        view.setAnimationEndTime(value.toDouble())
    }

    private fun resetAnimationEndTime(view: AnimatedImageView, animator: ValdiAnimator?) {
        view.setAnimationEndTime(0.0)
    }

    private fun applyOnProgress(view: AnimatedImageView, value: ValdiFunction) {
        view.setOnProgress(value)
    }

    private fun resetOnProgress(view: AnimatedImageView) {
        view.setOnProgress(null)
    }

    private fun applyObjectFit(view: AnimatedImageView, value: String, animator: ValdiAnimator?) {
        view.setScaleType(value)
    }

    private fun resetObjectFit(view: AnimatedImageView, animator: ValdiAnimator?) {
        view.setScaleType("contain")
    }

    private fun applyOnImageDecoded(view: AnimatedImageView, value: ValdiFunction) {
        view.onImageDecoded = value
    }

    private fun resetOnImageDecoded(view: AnimatedImageView) {
        view.onImageDecoded = null
    }

    override fun bindAttributes(attributesBindingContext: AttributesBindingContext<AnimatedImageView>) {
        attributesBindingContext.bindAssetAttributes(AssetOutputType.LOTTIE)

        attributesBindingContext.bindBooleanAttribute("loop", false, this::applyLoop, this::resetLoop)
        attributesBindingContext.bindFloatAttribute("advanceRate", false, this::applyAdvanceRate, this::resetAdvanceRate)
        attributesBindingContext.bindFloatAttribute("currentTime", false, this::applyCurrentTime, this::resetCurrentTime)
        attributesBindingContext.bindFloatAttribute("animationStartTime", false, this::applyAnimationStartTime, this::resetAnimationStartTime)
        attributesBindingContext.bindFloatAttribute("animationEndTime", false, this::applyAnimationEndTime, this::resetAnimationEndTime)
        attributesBindingContext.bindFunctionAttribute("onProgress", this::applyOnProgress, this::resetOnProgress)
        attributesBindingContext.bindStringAttribute("objectFit", false, this::applyObjectFit, this::resetObjectFit)
        attributesBindingContext.bindFunctionAttribute("onImageDecoded", this::applyOnImageDecoded, this::resetOnImageDecoded)
    }
}
