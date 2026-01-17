package com.snap.valdi.views

import android.content.Context
import com.snap.valdi.IValdiRuntime
import com.snap.valdi.InflationCompletion
import com.snap.valdi.context.ValdiViewOwner

open class ValdiGeneratedRootView<ViewModelType, ComponentContextType>(context: Context): ValdiRootView(context) {

    var viewModel: ViewModelType?
        get() = valdiContext?.viewModel as? ViewModelType
        set(value) {
            setViewModelUntyped(value)
        }

    val componentContext: ComponentContextType?
        get() = valdiContext?.componentContext?.get() as? ComponentContextType

    constructor(componentPath: String,
                runtime: IValdiRuntime,
                viewModel: ViewModelType?,
                componentContext: ComponentContextType?,
                owner: ValdiViewOwner? = null,
                inflationCompletion: InflationCompletion? = null) : this(runtime.context) {
        runtime.inflateViewAsync(this, componentPath, viewModel, componentContext, owner, inflationCompletion)
    }
}
