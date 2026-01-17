package com.snap.valdi.views

import android.content.Context
import android.graphics.Canvas
import android.graphics.Rect
import android.graphics.drawable.Drawable
import android.widget.ImageView
import androidx.annotation.Keep
import com.snap.valdi.callable.ValdiFunction
import com.snap.valdi.extensions.ViewUtils
import com.snap.valdi.logger.Logger
import com.snap.valdi.snapdrawing.AnimatedImage
import com.snap.valdi.snapdrawing.SnapDrawingOptions
import com.snap.valdi.snapdrawing.SnapDrawingRuntime
import com.snap.valdi.utils.CanvasClipper
import com.snap.valdi.utils.ValdiMarshaller
import com.snap.valdi.views.snapdrawing.SnapDrawingContainerView
import com.snapchat.client.valdi.utils.CppObjectWrapper

@Keep
class AnimatedImageView(snapDrawingOptions: SnapDrawingOptions,
                 logger: Logger,
                 factory: SnapDrawingRuntime,
                 context: Context): SnapDrawingContainerView(snapDrawingOptions, logger, factory, context), ValdiForegroundHolder,
        ValdiRecyclableView,
        ValdiClippableView,
        ValdiAssetReceiver {

    interface OnProgressCallback {
        /**
         * Called when the animation progress.
         * currentTime is the time of the animation in seconds.
         * duration is the duration of the animation in seconds.
         */
        fun onProgress(currentTime: Double, duration: Double)
    }

    override val clipper: CanvasClipper = CanvasClipper()

    override var clipToBounds: Boolean = false
    override val clipToBoundsDefaultValue: Boolean = false
    override var valdiForeground: Drawable? = null
    private val clipperBounds = Rect(0, 0, 0, 0)

    var onImageDecoded: ValdiFunction? = null

    init {
        nativeSetAnimatedImageLayerAsContentLayer(snapDrawingRootHandle.nativeHandle)
    }

    /**
     * Set the image to render.
     * If shouldDrawFlipped is true, the image will be drawn horizontally flipped.
     */
    fun setImage(image: AnimatedImage?, shouldDrawFlipped: Boolean) {
        nativeSetImage(snapDrawingRootHandle.nativeHandle, image?.handle?.nativeHandle ?: 0L, shouldDrawFlipped)
    }

    /**
     * Sets the speed ratio at which the image animation should run, 0 is paused, 1 means
     * the animation runs at normal speed, 0.5 at half speed, -1 the animation will run
     * in reverse.
     * Default is 0.
     */
    fun setAdvanceRate(advanceRate: Double) {
        nativeSetAdvanceRate(snapDrawingRootHandle.nativeHandle, advanceRate)
    }

    /**
     * Specify whether the image animation should loop back to the beginning
     * when reaching the end.
     */
    fun setShouldLoop(shouldLoop: Boolean) {
        nativeSetShouldLoop(snapDrawingRootHandle.nativeHandle, shouldLoop)
    }

    /**
     * Set the current time in seconds for the image animation
     */
    fun setCurrentTime(currentTime: Double) {
        nativeSetCurrentTime(snapDrawingRootHandle.nativeHandle, currentTime)
    }

    /**
     * Set the start time in seconds for the image animation
     * Animation will be clamped between start and end time
     */
    fun setAnimationStartTime(startTime: Double) {
        nativeSetAnimationStartTime(snapDrawingRootHandle.nativeHandle, startTime)
    }

    /**
     * Set the end time in seconds for the image animation
     * Animation will be clamped between start and end time
     */
    fun setAnimationEndTime(endTime: Double) {
        nativeSetAnimationEndTime(snapDrawingRootHandle.nativeHandle, endTime)
    }

    fun setScaleType(scaleType: String) {
        nativeSetScaleType(snapDrawingRootHandle.nativeHandle, scaleType)
    }

    /**
     * Set the progress callback that will be called when the animation progresses
     */
    fun setOnProgress(callback: ValdiFunction?) {
        nativeSetOnProgress(snapDrawingRootHandle.nativeHandle, callback)
    }

    /**
     * Set the progress callback that will be called when the animation progresses
     */
    fun setOnProgress(onProgressCallback: OnProgressCallback) {
        setOnProgress(object: ValdiFunction {
            override fun perform(marshaller: ValdiMarshaller): Boolean {
                val currentTime = marshaller.getDouble(0)
                val duration = marshaller.getDouble(1)

                onProgressCallback.onProgress(currentTime, duration)

                return false
            }
        })
    }

    override fun dispatchDraw(canvas: Canvas) {
        if (clipToBounds && canvas != null) {
            clipperBounds.right = width
            clipperBounds.bottom = height

            clipper.clip(canvas, clipperBounds)
        }

        ViewUtils.dispatchDraw(this, canvas) {
            super.dispatchDraw(canvas)
        }
    }

    override fun verifyDrawable(who: Drawable): Boolean {
        return ViewUtils.verifyDrawable(this, who) || super.verifyDrawable(who)
    }

    override fun onAssetChanged(asset: Any?, shouldFlip: Boolean) {
        val cppHandle = asset as? CppObjectWrapper

        val animatedImage: AnimatedImage? = if (cppHandle != null) {
            AnimatedImage(cppHandle)
        } else {
            null
        }

        setImage(animatedImage, shouldFlip)

        if (onImageDecoded != null && animatedImage != null) {
            ValdiMarshaller.use {
                it.pushDouble(animatedImage.getSize().width.toDouble())
                it.pushDouble(animatedImage.getSize().height.toDouble())

                onImageDecoded?.perform(it)
            }
        }
    }

    companion object {
        @JvmStatic
        private external fun nativeSetAnimatedImageLayerAsContentLayer(snapDrawingRootHandle: Long)

        @JvmStatic
        private external fun nativeSetImage(snapDrawingRootHandle: Long, sceneHandle: Long, shouldDrawFlipped: Boolean)

        @JvmStatic
        private external fun nativeSetAdvanceRate(snapDrawingRootHandle: Long, advanceRate: Double)

        @JvmStatic
        private external fun nativeSetShouldLoop(snapDrawingRootHandle: Long, shouldLoop: Boolean)

        @JvmStatic
        private external fun nativeSetCurrentTime(snapDrawingRootHandle: Long, currentTime: Double)

        @JvmStatic
        private external fun nativeSetOnProgress(snapDrawingRootHandle: Long, onProgress: Any?)

        @JvmStatic
        private external fun nativeSetAnimationStartTime(snapDrawingRootHandle: Long, startTime: Double)

        @JvmStatic
        private external fun nativeSetAnimationEndTime(snapDrawingRootHandle: Long, endTime: Double)

        @JvmStatic
        private external fun nativeSetScaleType(snapDrawingRootHandle: Long, scaleType: String)
    }
}
