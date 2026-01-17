package com.snap.valdi.modules

import com.snap.valdi.callable.ValdiFunction
import com.snap.valdi.utils.ValdiMarshaller
import com.snapchat.client.valdi_core.ModuleFactory

abstract class ValdiBridgeModule: ModuleFactory() {

    protected inline fun makeBridgeMethod(crossinline call: (marshaller: ValdiMarshaller) -> Unit): ValdiFunction {
        return object: ValdiFunction {
            override fun perform(marshaller: ValdiMarshaller): Boolean {
                call(marshaller)
                return true
            }
        }
    }

}