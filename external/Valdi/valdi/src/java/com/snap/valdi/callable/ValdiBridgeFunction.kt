package com.snap.valdi.callable

import com.snap.valdi.jsmodules.ValdiJSRuntime
import com.snap.valdi.schema.ValdiValueMarshallerRegistry
import com.snap.valdi.utils.ValdiMarshaller

open class ValdiBridgeFunction {

    companion object {
        @JvmStatic
        fun <T: ValdiBridgeFunction> createFromRuntime(runtime: ValdiJSRuntime,
                                                          cls: Class<T>,
                                                          modulePath: String): T {
            return ValdiMarshaller.use {
                val registry = ValdiValueMarshallerRegistry.shared
                registry.setActiveSchemaOfClassToMarshaller(cls, it)
                val objectIndex = runtime.pushModuleToMarshaller(modulePath, it)
                it.checkError()
                ValdiValueMarshallerRegistry.shared.unmarshallObject(cls, it, objectIndex)
            }
        }
    }

}
