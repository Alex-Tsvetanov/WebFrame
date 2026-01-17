package com.snap.valdi.attributes.impl

import android.view.ViewGroup
import com.snap.valdi.attributes.AttributesBinder
import com.snap.valdi.attributes.AttributesBindingContext

/**
 * Binds attributes for the ViewGroup's view class
 */
class ViewGroupAttributesBinder: AttributesBinder<ViewGroup> {

    override val viewClass: Class<ViewGroup>
        get() = ViewGroup::class.java

    override fun bindAttributes(attributesBindingContext: AttributesBindingContext<ViewGroup>) {
    }

}
