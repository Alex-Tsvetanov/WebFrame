package com.snap.valdi.utils

enum class ValdiAssetLoadOutputType(val value: Int) {
    /**
     * Expected ValdiImage output should expose its content as a BitmapHandler object
     */
    BITMAP(0x000001),

    /*
     * Expected ValdiImage output should expose its content as raw bytes or as
     * a local file path
     */
    RAW_CONTENT(0x000010),

    /**
     * Video
     */
    VIDEO(0x000100)
}

data class ValdiImageLoadOptions(val requestedWidth: Int,
                                    val requestedHeight: Int,
                                    val blurRadius: Float,
                                    val outputType: ValdiAssetLoadOutputType)