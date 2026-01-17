package com.snap.valdi.snapdrawing

import com.snap.valdi.attributes.impl.fonts.FontStyle
import com.snap.valdi.attributes.impl.fonts.FontWeight

interface SnapDrawingRuntime {
    fun getNativeHandle(): Long

    fun createRoot(disallowSynchronousDraw: Boolean): SnapDrawingRootHandle

    fun getSurfacePresenterFactory(): SurfacePresenterFactory

    fun registerTypeface(familyName: String,
                         fontWeight: FontWeight,
                         fontStyle: FontStyle,
                         isFallback: Boolean,
                         typefaceBytes: ByteArray?,
                         typefaceFd: Int?)

    fun getMaxRenderTargetSize(): Long
}
