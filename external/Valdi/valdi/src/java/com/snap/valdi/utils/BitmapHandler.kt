package com.snap.valdi.utils

import android.graphics.Bitmap
import androidx.annotation.Keep

@Keep
interface BitmapHandler {

    @Keep
    fun getBitmap(): Bitmap

    @Keep
    fun retain()

    @Keep
    fun release()
}
