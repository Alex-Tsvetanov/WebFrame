package com.snap.valdi.drawables

import android.content.Context
import android.graphics.Canvas
import android.graphics.Color
import android.graphics.ColorFilter
import android.graphics.PixelFormat
import android.graphics.drawable.Drawable
import android.widget.ImageView
import com.snap.valdi.drawables.utils.BorderRadii
import com.snap.valdi.drawables.utils.FixedDrawableInfoProvider
import com.snapchat.client.valdi_core.Asset
import com.snapchat.client.valdi_core.AssetLoadObserver
import com.snap.valdi.utils.CanvasClipper
import com.snap.valdi.utils.ValdiImage
import com.snapchat.client.valdi_core.AssetOutputType

/**
 */
class ValdiImageDrawable(
    context: Context,
    val listener: Listener?
):
    Drawable(),
    Drawable.Callback
{

    /**
     * Public interface
     */
    interface Listener {
        fun onLoadComplete()
        fun onLoadError(error: Throwable)

        fun isLayoutFinished(): Boolean
    }

    /**
     * Children renderables
     */
    private var currentPlaceholder: Drawable? = null
    private var currentDrawable: Drawable? = null
    private var currentAsset: Asset? = null
    private var isObservingAsset = false

    private var currentBitmapDrawable: ValdiBitmapDrawable? = null
    private var currentImage: ValdiImage? = null

    private var assetObserver: AssetLoadObserver? = null

    /**
     * Properties for configuring the image bitmap
     */
    val clipper: CanvasClipper = CanvasClipper()

    private var tint: Int = Color.TRANSPARENT

    var clipToBounds = true
        set(value) {
            field = value
            currentBitmapDrawable?.clipsToBound = value
        }

    var scaleType = ImageView.ScaleType.FIT_XY
        set(value) {
            field = value
            currentBitmapDrawable?.scaleType = value
        }

    var contentScaleX = 1.0f
        set(value) {
            field = value
            currentBitmapDrawable?.contentScaleX = value
        }

    var contentScaleY = 1.0f
        set(value) {
            field = value
            currentBitmapDrawable?.contentScaleY = value
        }

    var contentRotation = 0.0f
        set(value) {
            field = value
            currentBitmapDrawable?.contentRotation = value
        }    

    /**
     * Configuration of the drawable
     */
    var imagePadding = 0
        set(value) {
            if (field != value) {
                field = value
                invalidateSelf()
            }
        }

    var shouldFlip = false
        set(value) {
            if (field != value) {
                field = value
                invalidateSelf()
            }
        }

    fun setBorderRadii(borderRadii: BorderRadii?) {
        clipper.borderRadiiProvider = FixedDrawableInfoProvider(borderRadii, null)
        invalidateSelf()
    }

    /**
     * Loading pipeline entrypoints
     */
    fun setPlaceholder(placeholder: Drawable?) {
        usePlaceholder(placeholder)
    }

    fun setAsset(asset: Asset?) {
        if (this.currentAsset != asset) {
            val previousAsset = this.currentAsset
            this.currentAsset = asset

            releaseBitmapHandler()

            if (isObservingAsset) {
                isObservingAsset = false
                previousAsset?.removeLoadObserver(assetObserver)
            }

            loadAsset(false)
        }
    }

    fun setDrawable(drawable: Drawable) {
        if (currentDrawable != drawable) {
            setAsset(null)
            useDrawable(currentDrawable, drawable)
            currentDrawable = drawable
        }
    }

    fun setImage(image: ValdiImage?) {
        if (currentImage != image) {
            createBitmapDrawable()
            currentBitmapDrawable?.bitmap = image?.getContentAsBitmap()
            currentBitmapDrawable?.colorMatrix = image?.colorMatrix
            currentImage = image
            useDrawable(null, currentBitmapDrawable)
        }
    }

    fun getWidth(): Int? {
        return currentBitmapDrawable?.bitmap?.width
    }

    fun getHeight(): Int? {
        return currentBitmapDrawable?.bitmap?.height
    }

    fun onLayoutCompleted() {
        loadAsset(true)
    }

    /**
     * Loading pipeline, image -> bitmap
     */
    private fun loadAsset(forceLoading: Boolean) {
        val asset = this.currentAsset ?: return

        val isLayoutFinished = listener?.isLayoutFinished() ?: true
        if (isLayoutFinished || forceLoading) {
            val width = bounds.width()
            val height = bounds.height()

            if (!isObservingAsset) {
                isObservingAsset = true
                if (assetObserver == null) {
                    assetObserver = object: AssetLoadObserver() {
                        override fun onLoad(asset: Asset?, loadedAsset: Any?, error: String?) {
                            if (asset !== currentAsset) {
                                return@onLoad
                            }

                            if (error != null) {
                                listener?.onLoadError(Exception("Asset load error ${currentAsset}: $error.message"))
                            } else {
                                setImage(loadedAsset as? ValdiImage)
                                listener?.onLoadComplete()
                            }
                        }
                    }
                }
                asset.addLoadObserver(assetObserver, AssetOutputType.IMAGEANDROID, width, height, null)
            }
        }
    }

    /**
     * Setup rendering for child drawables
     */
    private fun usePlaceholder(placeholder: Drawable?) {
        useDrawable(currentPlaceholder, placeholder)
        currentPlaceholder = placeholder
    }

    private fun useDrawable(oldDrawable: Drawable?, newDrawable: Drawable?) {
        if (oldDrawable?.callback === this) {
            oldDrawable.callback = null
        }
        if (newDrawable != null) {
            newDrawable.callback = this
        }
        this.invalidateSelf()
    }

    /**
     * Bitmap handling
     */
    private fun createBitmapDrawable() {
        if (currentBitmapDrawable == null) {
            currentBitmapDrawable = ValdiBitmapDrawable(clipper).also {
                it.clipsToBound = clipToBounds
                it.scaleType = scaleType
                it.contentScaleX = contentScaleX
                it.contentScaleY = contentScaleY
                it.contentRotation = contentRotation
                it.setTint(tint)
            }
        }
    }

    private fun releaseBitmapHandler() {
        val currentBitmapHandler = this.currentImage
        if (currentBitmapHandler != null) {
            this.currentImage = null
            currentBitmapDrawable?.bitmap = null
            invalidateSelf()
        }
    }

    /**
     * Drawing
     */
    override fun draw(canvas: Canvas) {
        val drawable = resolveCurrentDrawable()
        if (drawable == null) {
            return
        }

        val bounds = getBounds()
        val left = bounds.left + imagePadding
        val top = bounds.top + imagePadding
        val right = Math.max(bounds.right - imagePadding, left)
        val bottom = Math.max(bounds.bottom - imagePadding, top)

        drawable.setBounds(left, top, right, bottom)
        drawable.draw(canvas)
    }

    private fun resolveCurrentDrawable(): Drawable? {
        if (currentImage != null) {
            currentBitmapDrawable?.flip = shouldFlip
            return currentBitmapDrawable
        }

        if (currentDrawable != null) {
            return currentDrawable
        }

        return currentPlaceholder
    }

    /**
     * Drawable.Callbacks overrides
     */
    override fun invalidateDrawable(drawable: Drawable) {
        this.invalidateSelf()
    }

    override fun scheduleDrawable(drawable: Drawable, runnable: Runnable, duration: Long) {
        this.scheduleSelf(runnable, duration)
    }

    override fun unscheduleDrawable(drawable: Drawable, runnable: Runnable) {
        this.unscheduleSelf(runnable)
    }

    /**
     * Drawable overrides
     */
    override fun setAlpha(alpha: Int) {
        // Unsupported
    }

    override fun setColorFilter(colorFilter: ColorFilter?) {
        // Unsupported
    }

    override fun setTint(value: Int) {
        this.tint = value
        currentBitmapDrawable?.setTint(value)
    }

    override fun getOpacity(): Int {
        return PixelFormat.TRANSPARENT
    }

}
