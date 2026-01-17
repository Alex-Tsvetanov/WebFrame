package com.snap.valdi.utils

import android.graphics.Bitmap
import com.snap.valdi.exceptions.ValdiException

class ValdiImageWithBitmap(private var bitmap: Bitmap?): ValdiImage() {

    override fun getBitmap(): Bitmap {
        return bitmap ?: throw ValdiException("Bitmap was disposed")
    }

    override fun onDestroyBitmap() {
        val bitmap = this.bitmap
        this.bitmap = null

        bitmap?.recycle()
    }
}