package com.snap.valdi.utils

import android.content.Context
import android.graphics.Bitmap
import androidx.annotation.Keep
import com.snap.valdi.bundle.LocalAssetLoader
import com.snap.valdi.exceptions.ValdiException
import java.io.IOException
import java.lang.UnsupportedOperationException
import java.util.concurrent.atomic.AtomicInteger

@Keep
abstract class ValdiImage: BitmapHandler {

    private val retainCount = AtomicInteger(0)

    val isUnused: Boolean
        get() = retainCount.get() == 0

    private var destroyed = false

    var colorMatrix: FloatArray? = null

    private fun destroy() {
        val shouldDestroy = synchronized(this) {
            if (destroyed) {
                false
            } else {
                destroyed = true
                true
            }
        }

        if (shouldDestroy) {
            onDestroyBitmap()
        }
    }

    fun getContentAsBitmap(): Bitmap? {
        return try {
            // TODO(simon): Remove once getBitmap() is completely removed
            getBitmap()
        } catch (exc: UnsupportedOperationException) {
            (getContent() as? ValdiImageContent.Bitmap)?.bitmap
        }
    }

    // TODO(simon): Remove inheritance of BitmapHandler from ValdiImage
    override fun getBitmap(): Bitmap {
        throw UnsupportedOperationException()
    }

    /**
     * Returns the content of the ValdiImage.
     * If the ValdiImage does not hold a Bitmap,
     * this method should be overridden to return the
     * right content through this method.
     */
    open fun getContent(): ValdiImageContent {
        return ValdiImageContent.Bitmap(getBitmap())
    }

    @Keep
    fun onRetrieveContent(marshallerHandle: Long): Any? {
        return BridgeCallUtils.handleCall(marshallerHandle) {
            try {
                when (val content = getContent()) {
                    is ValdiImageContent.Bitmap -> null /* Null is treated as a Bitmap */
                    is ValdiImageContent.FileReference -> content.filePath
                    is ValdiImageContent.Bytes -> content.bytes
                }
            } catch (exc: IOException) {
                throw ValdiException("Failed to get image content", exc)
            }
        }
    }

    protected abstract fun onDestroyBitmap()

    override fun retain() {
        retainCount.incrementAndGet()
    }

    override fun release() {
        val newValue = retainCount.decrementAndGet()
        if (newValue == 0) {
            destroy()
        }
    }

    companion object {

        // Resolving resource id by name is an expensive call, do it only once per resource.
        @JvmStatic
        fun getImageIdentifier(context: Context, moduleName: String, resourceName: String): Int {
            val cleanedResourceName = resourceName.replace('-', '_')

            var identifier = context.resources.getIdentifier("${moduleName}_$cleanedResourceName", "drawable", context.packageName)
            if (identifier == 0) {
                identifier = context.resources.getIdentifier(cleanedResourceName, "drawable", context.packageName)
            }

            return identifier
        }

        /**
         * Returns a String representing an image URL that points to the given Android Image Resource Id.
         * The returned string can then be used as a "src" attribute for an "image" element in TypeScript.
         */
        @JvmStatic
        fun getUrlStringForResId(resId: Int): String {
            return "${LocalAssetLoader.VALDI_ASSET_SCHEME}://${resId}"
        }
    }

}
