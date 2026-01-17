package com.snap.valdi.utils

import java.io.InputStream

sealed class ValdiImageContent {
    class Bitmap(val bitmap: android.graphics.Bitmap): ValdiImageContent()
    class FileReference(val filePath: String): ValdiImageContent()
    class Bytes(val bytes: ByteArray): ValdiImageContent()

    companion object {
        @JvmStatic
        fun fromStream(stream: InputStream): ValdiImageContent {
            return Bytes(stream.use {
                stream.readBytes()
            })
        }
    }
}
