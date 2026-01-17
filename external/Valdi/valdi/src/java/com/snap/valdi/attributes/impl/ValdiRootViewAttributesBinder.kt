package com.snap.valdi.attributes.impl

import com.snap.valdi.attributes.AttributesBinder
import com.snap.valdi.attributes.AttributesBindingContext
import com.snap.valdi.callable.ValdiFunction
import com.snap.valdi.views.ValdiRootView

/**
 * Binds attributes for the ValdiView's view class
 */
class ValdiRootViewAttributesBinder: AttributesBinder<ValdiRootView> {

    override val viewClass: Class<ValdiRootView>
        get() = ValdiRootView::class.java

    override fun bindAttributes(attributesBindingContext: AttributesBindingContext<ValdiRootView>) {
        attributesBindingContext.bindFunctionAttribute("onBackButtonPressed", this::applyOnBackButtonPressed, this::resetOnBackButtonPressed)
    }

    private fun resetOnBackButtonPressed(view: ValdiRootView) {
        view.setBackButtonObserverWithFunction(null)
    }

    private fun applyOnBackButtonPressed(view: ValdiRootView, action: ValdiFunction) {
        view.setBackButtonObserverWithFunction(action)
    }
}
