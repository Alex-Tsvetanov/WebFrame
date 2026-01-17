package com.snap.valdi.attributes.impl

import android.content.Context
import android.graphics.Color
import android.widget.ImageView
import com.snap.valdi.attributes.AttributesBinder
import com.snap.valdi.attributes.AttributesBindingContext
import com.snap.valdi.attributes.impl.animations.ValdiAnimator
import com.snap.valdi.callable.ValdiFunction
import com.snap.valdi.exceptions.AttributeError
import com.snap.valdi.views.ValdiImageView
import com.snapchat.client.valdi_core.AssetOutputType

class ValdiImageViewAttributesBinder(val context: Context): AttributesBinder<ValdiImageView> {

    override val viewClass: Class<ValdiImageView>
        get() = ValdiImageView::class.java

    fun applyObjectFit(view: ValdiImageView, value: String, animator: ValdiAnimator?) {
        val scaleType = when (value) {
            "none" -> ImageView.ScaleType.CENTER
            "fill" -> ImageView.ScaleType.FIT_XY
            "cover" -> ImageView.ScaleType.CENTER_CROP
            "contain" -> ImageView.ScaleType.CENTER_INSIDE
            else -> throw AttributeError("Unsupported cover value")
        }
        view.setScaleType(scaleType)
    }

    fun resetObjectFit(view: ValdiImageView, animator: ValdiAnimator?) {
        val scaleType = ImageView.ScaleType.FIT_XY
        view.setScaleType(scaleType)
    }

    fun applyTint(view: ValdiImageView, color: Int, animator: ValdiAnimator?) {
        view.setTint(color)
    }

    fun resetTint(view: ValdiImageView, animator: ValdiAnimator?) {
        view.setTint(Color.TRANSPARENT)
    }

    fun applyContentScaleX(view: ValdiImageView, value: Float, animator: ValdiAnimator?) {
        view.setContentScaleX(value)
    }

    fun resetContentScaleX(view: ValdiImageView, animator: ValdiAnimator?) {
        view.setContentScaleX(1.0f)
    }

    fun applyContentScaleY(view: ValdiImageView, value: Float, animator: ValdiAnimator?) {
        view.setContentScaleY(value)
    }

    fun resetContentScaleY(view: ValdiImageView, animator: ValdiAnimator?) {
        view.setContentScaleY(1.0f)
    }

    fun applyContentRotation(view: ValdiImageView, value: Float, animator: ValdiAnimator?) {
        view.setContentRotation(value)
    }

    fun resetContentRotation(view: ValdiImageView, animator: ValdiAnimator?) {
        view.setContentRotation(0.0f)
    }

    private fun applyOnImageDecoded(view: ValdiImageView, fn: ValdiFunction) {
        view.onImageDecoded = fn
    }

    private fun resetOnImageDecoded(view: ValdiImageView) {
        view.onImageDecoded = null
    }

    override fun bindAttributes(attributesBindingContext: AttributesBindingContext<ValdiImageView>) {
        attributesBindingContext.bindAssetAttributes(AssetOutputType.IMAGEANDROID)

        attributesBindingContext.bindStringAttribute("objectFit", false, this::applyObjectFit, this::resetObjectFit)
        attributesBindingContext.bindColorAttribute("tint", false, this::applyTint, this::resetTint)

        attributesBindingContext.bindFloatAttribute("contentScaleX", false, this::applyContentScaleX, this::resetContentScaleX)
        attributesBindingContext.bindFloatAttribute("contentScaleY", false, this::applyContentScaleY, this::resetContentScaleY)
        attributesBindingContext.bindFloatAttribute("contentRotation", false, this::applyContentRotation, this::resetContentRotation)

        attributesBindingContext.bindFunctionAttribute("onImageDecoded", this::applyOnImageDecoded, this::resetOnImageDecoded)
    }

}
