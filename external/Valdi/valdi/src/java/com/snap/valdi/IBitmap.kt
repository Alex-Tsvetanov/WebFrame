package com.snap.valdi

import android.graphics.Bitmap
import androidx.annotation.Keep
import com.snap.valdi.schema.ValdiUntypedClass
import com.snap.valdi.utils.ValdiMarshallable
import com.snap.valdi.utils.ValdiMarshaller
import com.snapchat.client.valdi.utils.CppObjectWrapper
import com.snapchat.client.valdi.NativeBridge

@ValdiUntypedClass
@Keep
interface IBitmap: ValdiMarshallable {

    private class BitmapWithNativeHandle(private val nativeHandle: CppObjectWrapper): IBitmap {
        override fun pushToMarshaller(marshaller: ValdiMarshaller): Int {
            return marshaller.pushCppObject(nativeHandle)
        }
    }

    companion object {
        @JvmStatic
        fun fromAndroidBitmap(bitmap: Bitmap): IBitmap {
            val handle = NativeBridge.wrapAndroidBitmap(bitmap) as CppObjectWrapper
            return BitmapWithNativeHandle(handle)
        }
    }
}
