package com.snap.valdi.attributes.impl.fonts

import android.content.res.AssetFileDescriptor

interface FontDataProvider {
    fun openAssetFileDescriptor(): AssetFileDescriptor?
    fun getBytes(): ByteArray?
}