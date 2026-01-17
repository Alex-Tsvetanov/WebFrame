package com.snap.valdi.promise

import androidx.annotation.Keep
import com.snap.valdi.exceptions.messageWithCauses
import com.snap.valdi.utils.CppNativeHandlePair

@Keep
class CppPromiseCallback<T>(nativeHandle: Long, valueMarshallerNativeHandle: Long):
    CppNativeHandlePair(nativeHandle, valueMarshallerNativeHandle), PromiseCallback<T> {

    override fun onSuccess(value: T) {
        // We swap because our native call is going to release the handle for us.
        nativeOnSuccess(this.swapNativeHandle1(), this.swapNativeHandle2(), value)
    }

    override fun onFailure(error: Throwable) {
        nativeOnFailure(this.swapNativeHandle1(), this.swapNativeHandle2(), error.messageWithCauses())
    }

    companion object {
        @JvmStatic
        private external fun nativeOnSuccess(nativeHandle: Long, valueMarshallerNativeHandle: Long, value: Any?)
        @JvmStatic
        private external fun nativeOnFailure(nativeHandle: Long, valueMarshallerNativeHandle: Long, error: String)
    }
}