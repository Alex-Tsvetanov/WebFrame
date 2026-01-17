package com.snap.valdi.promise

import androidx.annotation.Keep
import com.snap.valdi.utils.CppNativeHandlePair

@Keep
class CppPromise<T>(nativeHandle: Long, valueMarshallerNativeHandle: Long):
    CppNativeHandlePair(nativeHandle, valueMarshallerNativeHandle), Promise<T> {

    override fun onComplete(callback: PromiseCallback<T>) {
        nativeOnComplete(nativeHandle1, nativeHandle2, callback)
    }

    override fun cancel() {
        nativeCancel(nativeHandle1)
    }

    override fun isCancelable(): Boolean {
        return nativeIsCancelable(nativeHandle1)
    }

    companion object {
        @JvmStatic
        private external fun nativeOnComplete(nativeHandle: Long, valueMarshallerNativeHandle: Long, completion: Any)
        @JvmStatic
        private external fun nativeCancel(nativeHandle: Long)
        @JvmStatic
        private external fun nativeIsCancelable(nativeHandle: Long): Boolean
    }
}