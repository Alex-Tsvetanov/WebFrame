package com.snap.valdi.callable

import androidx.annotation.Keep
import com.snap.valdi.actions.ValdiAction
import com.snap.valdi.utils.ValdiMarshaller

/**
 * Takes a ValdiAction and adapts it into a ValdiFunction
 */
@Keep
class ValdiFunctionActionAdapter(val action: ValdiAction): ValdiFunction {

    override fun perform(marshaller: ValdiMarshaller): Boolean {
        val params = Array<Any?>(marshaller.size) {
            marshaller.getUntyped(it)
        }

        val value = action.perform(params)
        marshaller.pushUntyped(value)
        return true
    }

}