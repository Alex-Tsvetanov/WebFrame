package com.snap.valdi.attributes

import android.view.View

/**
 * An AttributesBinder can bind attributes on a specific view class.
 * Its bindAttributes() method is called once by Valdi the first time
 * a view of the specified viewClass is being inflated.
 * If you want to expose new attributes to Valdi, you should create an object
 * that implements this interface and register it using Runtime's registerGlobalAttributesBinder().
 *
 * If your AttributesBinder requires some dependencies that are local to your feature, you'll want to
 * avoid registering it as a global attributes binder. Instead, you can register it with your Valdi
 * view's stack. To do so, you should create your root Valdi view first and then do:
 *     yourValdiView.getValdiContext {
 *         it.registerAttributesBinder(yourSpecialAttributeBinder)
 *     }
}
 */
interface AttributesBinder<T: View> {

    val viewClass: Class<T>

    /**
     * Asks the AttributesBinder to bind all attributes this AttributesBinder knows about.
     */
    fun bindAttributes(attributesBindingContext: AttributesBindingContext<T>)

}
