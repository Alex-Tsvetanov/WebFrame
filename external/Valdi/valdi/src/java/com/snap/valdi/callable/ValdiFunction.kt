package com.snap.valdi.callable

import androidx.annotation.Keep
import com.snap.valdi.utils.ValdiMarshaller

@Keep
interface ValdiFunction {

    fun perform(marshaller: ValdiMarshaller): Boolean

}

fun ValdiFunction.safePerform(marshaller: ValdiMarshaller) {
    if (!perform(marshaller)) {
        marshaller.pushUndefined()
    }
}
