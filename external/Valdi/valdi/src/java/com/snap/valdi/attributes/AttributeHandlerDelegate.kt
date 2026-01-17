package com.snap.valdi.attributes

import android.view.View
import com.snap.valdi.attributes.impl.animations.ValdiAnimator

abstract class AttributeHandlerDelegate {
    // Set after registration to identify the attribute name
    var attributeId: Int = 0
    var attributeName = ""
    var viewClassName = ""

    val applyAttributeTrace: String
        get() {
            if (applyAttributeTraceField == null) {
                applyAttributeTraceField = "Valdi.applyAttribute.${viewClassName}.${attributeName}"
            }
            return applyAttributeTraceField!!
        }

    val resetAttributeTrace: String
        get() {
            if (resetAttributeTraceField == null) {
                resetAttributeTraceField = "Valdi.resetAttribute.${viewClassName}.${attributeName}"
            }
            return resetAttributeTraceField!!
        }

    private var applyAttributeTraceField: String? = null
    private var resetAttributeTraceField: String? = null

    abstract fun onReset(view: View, animator: ValdiAnimator?)
}

abstract class BooleanAttributeHandlerDelegate: AttributeHandlerDelegate() {
    abstract fun onApply(view: View, value: Boolean, animator: ValdiAnimator?)
}

abstract class IntAttributeHandlerDelegate: AttributeHandlerDelegate() {
    abstract fun onApply(view: View, value: Int, animator: ValdiAnimator?)
}

abstract class LongAttributeHandlerDelegate: AttributeHandlerDelegate() {
    abstract fun onApply(view: View, value: Long, animator: ValdiAnimator?)
}

abstract class FloatAttributeHandlerDelegate: AttributeHandlerDelegate() {
    abstract fun onApply(view: View, value: Float, animator: ValdiAnimator?)
}

abstract class ObjectAttributeHandlerDelegate: AttributeHandlerDelegate() {
    abstract fun onApply(view: View, value: Any?, animator: ValdiAnimator?)
}

abstract class PercentAttributeHandlerDelegate: AttributeHandlerDelegate() {
    abstract fun onApply(view: View, value: Float, valueIsPercent: Boolean, animator: ValdiAnimator?)
}

abstract class CornersAttributeHandlerDelegate: AttributeHandlerDelegate() {
    abstract fun onApply(view: View,
                topLeft: Float,
                topLeftIsPercent: Boolean,
                topRight: Float,
                topRightIsPercent: Boolean,
                bottomRight: Float,
                bottomRightIsPercent: Boolean,
                bottomLeft: Float,
                bottomLeftIsPercent: Boolean,
                animator: ValdiAnimator?)
}
