package com.snap.valdi.utils

import android.net.Uri
import com.snap.valdi.exceptions.ValdiException

/**
 * A ValdiAssetLoader implementation is responsible to load Assets.
 */
interface ValdiAssetLoader {

    /**
    Returns the URL schemes supported by this ImageLoader
     */
    fun getSupportedURLSchemes(): List<String>

    /**
     * Returns a RequestPayload that will be provided to the load function of the implementation.
     * The request payload might be cached by the runtime to avoid processing
     * it again. This method can throw a ValdiException if Uri cannot be parsed.
     */
    @Throws(ValdiException::class)
    fun getRequestPayload(url: Uri): Any

    /**
     * Returns the output types that this ValdiAssetLoader supports.
     */
    fun getSupportedOutputTypes(): Int
}
