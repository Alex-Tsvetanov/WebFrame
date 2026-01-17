package com.snap.valdi.views

import com.snap.valdi.attributes.impl.richtext.TextViewHelper

/**
 * Views that inherit TextView must implement this interface
 * so that the TextViewAttributesBinder can inject the TextViewHelper.
 * Subclasses should also call update() on the helper in the onMeasure() method.
 */
interface ValdiTextHolder {

    var textViewHelper: TextViewHelper?

    fun setTextAccessibility(text: CharSequence?)

}