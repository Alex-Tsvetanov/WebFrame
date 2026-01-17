package com.snap.valdi.snapdrawing

import android.graphics.Bitmap
import android.util.SizeF
import com.snapchat.client.valdi.utils.CppObjectWrapper
import java.io.InputStream


/**
 * An animated image's binary representation.
 */
class AnimatedImage(val handle: CppObjectWrapper) {

    /**
     * Draw the scene into the given Bitmap at the given time.
     * @param bitmap The bitmap that the scene should be rasterized into
     * @param x the x position inside the bitmap on which the scene should be drawn
     * @param y the y position inside the bitmap on which the scene should be drawn
     * @param width the width of the scene inside the bitmap
     * @param height the width of the scene inside the bitmap
     * @param timeSeconds the time in the seconds that should be rendered
     */
    fun drawInBitmap(bitmap: Bitmap, x: Int, y: Int, width: Int, height: Int, timeSeconds: Double) {
        nativeDrawInBitmap(handle.nativeHandle, bitmap, x, y, width, height, timeSeconds)
    }

    fun getDuration(): Double {
        return nativeGetDuration(handle.nativeHandle)
    }

    fun getFrameRate(): Double {
        return nativeGetFrameRate(handle.nativeHandle)
    }

    fun getSize(): SizeF {
        val sizeArray = nativeGetSize(handle.nativeHandle) as? FloatArray ?: return SizeF(0f, 0f)
        return SizeF(sizeArray[0], sizeArray[1])
    }

    companion object {
        @JvmStatic
        private external fun nativeParse(runtimeOrFontManagerHandle: Long, sceneData: ByteArray, isFontManager: Boolean): Any?

        @JvmStatic
        private external fun nativeDrawInBitmap(nativeHandle: Long, bitmap: Any?, x: Int, y: Int, width: Int, height: Int, timeSeconds: Double)

        @JvmStatic
        private external fun nativeGetDuration(nativeHandle: Long): Double

        @JvmStatic
        private external fun nativeGetFrameRate(nativeHandle: Long): Double

        @JvmStatic
        private external fun nativeGetSize(nativeHandle: Long): Any?

        /**
         * Parse an animated image or Lottie Scene in JSON format from an InputStream.
         * Throws a ValdiException on failure.
         * Returns a AnimatedImage instance which can be played back on a AnimatedImageView.
         */
        @JvmStatic
        fun parse(runtime: SnapDrawingRuntime, inputStream: InputStream): AnimatedImage {
            return parse(runtime, inputStream.readBytes())
        }

        /**
         * Parse an image or Lottie Scene in JSON format from a byte array.
         * Throws a ValdiException on failure.
         * Returns a AnimatedImage instance which can be played back on a AnimatedImageView.
         */
        @JvmStatic
        fun parse(runtime: SnapDrawingRuntime, imageData: ByteArray): AnimatedImage {
            val nativeHandle = nativeParse(runtime.getNativeHandle(), imageData, false) as CppObjectWrapper
            return AnimatedImage(nativeHandle)
        }

        /**
         * Parse an image or Lottie Scene in JSON format from an InputStream.
         * Throws a ValdiException on failure.
         * Returns a AnimatedImage instance which can be played back on a AnimatedImageView.
         */
        @JvmStatic
        fun parse(fontManager: SnapDrawingFontManager, inputStream: InputStream): AnimatedImage {
            return parse(fontManager, inputStream.readBytes())
        }

        /**
         * Parse an image or Lottie Scene in JSON format from a byte array.
         * Throws a ValdiException on failure.
         * Returns a AnimatedImage instance which can be played back on a AnimatedImageView.
         */
        @JvmStatic
        fun parse(fontManager: SnapDrawingFontManager, sceneData: ByteArray): AnimatedImage {
            val nativeHandle = nativeParse(fontManager.getNativeHandle(), sceneData, true) as CppObjectWrapper
            return AnimatedImage(nativeHandle)
        }
    }

}
