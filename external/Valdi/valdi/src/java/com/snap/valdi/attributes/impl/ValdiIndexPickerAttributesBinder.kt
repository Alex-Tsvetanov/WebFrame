package com.snap.valdi.attributes.impl

import android.content.Context
import android.graphics.Color
import android.util.TypedValue
import android.view.View
import android.view.ViewGroup
import com.snap.valdi.attributes.AttributesBinder
import com.snap.valdi.attributes.AttributesBindingContext
import com.snap.valdi.attributes.conversions.ColorConversions
import com.snap.valdi.attributes.impl.animations.ColorAnimator
import com.snap.valdi.attributes.impl.animations.ValdiAnimator
import com.snap.valdi.attributes.impl.animations.ViewAnimator
import com.snap.valdi.attributes.impl.gestures.GestureAttributes
import com.snap.valdi.drawables.ValdiGradientDrawable
import com.snap.valdi.exceptions.AttributeError
import com.snap.valdi.extensions.ViewUtils
import com.snap.valdi.logger.Logger
import com.snap.valdi.utils.CoordinateResolver
import com.snap.valdi.utils.JSConversions
import com.snap.valdi.utils.error
import com.snap.valdi.views.ValdiIndexPicker
import com.snap.valdi.views.ValdiClippableView
import com.snap.valdi.views.ValdiForegroundHolder
import com.snapchat.client.valdi_core.AttributeType
import com.snapchat.client.valdi_core.CompositeAttributePart
import com.snap.valdi.callable.ValdiFunction

class ValdiIndexPickerAttributesBinder(private val context: Context, val logger: Logger) : AttributesBinder<ValdiIndexPicker> {

    override val viewClass: Class<ValdiIndexPicker>
        get() = ValdiIndexPicker::class.java

    fun applyContent(view: ValdiIndexPicker, value: Any?, animator: ValdiAnimator?) {
        if (value !is Array<*>) {
            throw AttributeError("content should be an array")
        }
        if (value.size != 2) {
            throw AttributeError("content should have 2 values in the given array")
        }
        val index = value[0] as? Double
        val labels = value[1] as? Array<Any>

        view.setContent(index?.toInt(), labels?.map({ it.toString() })?.toTypedArray())
    }

    fun resetContent(view: ValdiIndexPicker, animator: ValdiAnimator?) {
        view.setContent(null, null)
    }

    private fun applyOnChange(view: ValdiIndexPicker, value: ValdiFunction) {
        view.onChange = value
    }

    private fun resetOnChange(view: ValdiIndexPicker) {
        view.onChange = null
    }

    override fun bindAttributes(attributesBindingContext: AttributesBindingContext<ValdiIndexPicker>) {
        attributesBindingContext.bindCompositeAttribute("content", arrayListOf<CompositeAttributePart>(
                CompositeAttributePart("index", AttributeType.DOUBLE, true, false),
                CompositeAttributePart("labels", AttributeType.UNTYPED, false, true)
        ), this::applyContent, this::resetContent)
        attributesBindingContext.bindFunctionAttribute("onChange", this::applyOnChange, this::resetOnChange)

        attributesBindingContext.setPlaceholderViewMeasureDelegate(lazy {
            ValdiIndexPicker(context).apply {
                layoutParams = ViewGroup.LayoutParams(
                        ViewGroup.LayoutParams.WRAP_CONTENT,
                        ViewGroup.LayoutParams.WRAP_CONTENT
                )
            }
        })
    }
}
