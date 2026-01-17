package com.snap.valdi.bundle

import com.snap.valdi.exceptions.ValdiFatalException
import com.snap.valdi.exceptions.messageWithCauses
import com.snap.valdi.utils.ValdiImage
import com.snap.valdi.utils.ValdiImageLoadCompletion
import com.snap.valdi.utils.NativeRef
import com.snapchat.client.valdi.NativeBridge

class AssetImageLoaderCompletion(handle: Long,
                                 private val colorMatrixFilter: FloatArray?): NativeRef(handle), ValdiImageLoadCompletion {

    override fun onImageLoadComplete(imageWidth: Int,
                                     imageHeight: Int,
                                     image: ValdiImage?,
                                     error: Throwable?) {
        if (image != null) {
            image.colorMatrix = colorMatrixFilter

            image.retain()
            if (!NativeBridge.notifyAssetLoaderCompleted(nativeHandle, image, null, /* isImage */ true)) {
                image.release()
            }
        } else if (error != null) {
            NativeBridge.notifyAssetLoaderCompleted(nativeHandle, null, error.messageWithCauses(), /* isImage */ true)
        } else {
            throw ValdiFatalException("ImageLoadCompletion should submit either an image or an error")
        }

        // Dispose as soon as the first onImageLoadComplete is called, so that we can destroy the
        // native completion handler potentially earlier.
        dispose()
    }

}