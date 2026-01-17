package com.snap.valdi.views

import android.content.Context
import android.graphics.Canvas
import android.graphics.drawable.Drawable
import android.net.Uri
import android.view.View
import android.widget.ImageView
import com.snap.valdi.ValdiRuntime
import com.snapchat.client.valdi_core.Asset
import com.snap.valdi.callable.ValdiFunction
import com.snap.valdi.logger.Logger
import com.snap.valdi.utils.ValdiMarshaller
import com.snap.valdi.utils.warn
import com.snap.valdi.extensions.ViewUtils
import com.snap.valdi.utils.CanvasClipper
import com.snap.valdi.utils.CoordinateResolver
import com.snap.valdi.drawables.ValdiImageDrawable
import com.snap.valdi.utils.ValdiImage

/**
 * Base implementation for an efficient ImageView.
 * Need to be subclassed to provide an ImageLoader instance.
 */
open class ValdiImageView(context: Context): View(context), ValdiForegroundHolder,
        ValdiImageDrawable.Listener,
        ValdiRecyclableView,
        ValdiClippableView,
        ValdiAssetReceiver {

    /**
     * Dependencies
     */
    private val coordinateResolver = CoordinateResolver(context)

    private val runtime: ValdiRuntime?
        get() {
            return ViewUtils.findValdiContext(this)?.runtimeOrNull
        }

    private val logger: Logger?
        get() {
            return runtime?.logger
        }

    val imageDrawable = ValdiImageDrawable(context, this).also {
        it.callback = this
    }

    var onImageDecoded: ValdiFunction? = null

    /**
     * Proxy properties for setting up the image drawable
     */
    override val clipper: CanvasClipper
        get() {
            return imageDrawable.clipper
        }

    override var clipToBounds: Boolean
        get() {
            return imageDrawable.clipToBounds
        }
        set(value) {
            imageDrawable.clipToBounds = value
        }

    var imagePadding: Int
        get() {
            return imageDrawable.imagePadding
        }
        set(value) {
            imageDrawable.imagePadding = value
        }

    /**
     * Valdi view overrides
     */
    override val clipToBoundsDefaultValue = true

    private var valdiForegroundField: Drawable? = null
    override var valdiForeground: Drawable?
        get() {
            return valdiForegroundField
        }
        set(value) {
            valdiForegroundField = value
        }

    /**
     * State properties
     */
    private var currentAsset: Asset? = null

    /**
     * Loading inputs
     */
    fun setAsset(asset: Asset?) {
        this.currentAsset = asset
        imageDrawable.setBounds(0, 0, width, height)
        imageDrawable.setAsset(asset)
    }

    fun setUrlString(url: String) {
        setAsset(runtime?.getUrlAsset(url))
    }

    fun setUri(uri: Uri) {
        setUrlString(uri.toString())
    }

    fun setDrawable(drawable: Drawable) {
        setAsset(null)
        imageDrawable.setDrawable(drawable)
    }

    override fun onAssetChanged(asset: Any?, shouldFlip: Boolean) {
        imageDrawable.shouldFlip = shouldFlip
        imageDrawable.setImage(asset as? ValdiImage)

        if (onImageDecoded != null && asset != null) {
            ValdiMarshaller.use {
                it.pushDouble(imageDrawable.getWidth()?.toDouble() ?: -1.0)
                it.pushDouble(imageDrawable.getHeight()?.toDouble() ?: -1.0)

                onImageDecoded?.perform(it)
            }
        }
    }

    /**
     * Loading outputs
     */
    override fun onLoadComplete() {
    }

    override fun onLoadError(error: Throwable) {
        logger?.warn("Failed to load: $error.message")
    }

    override fun isLayoutFinished(): Boolean {
        return !isLayoutRequested && getParent() != null
    }

    override fun draw(canvas: Canvas) {
        super.draw(canvas)
    }

    /**
     * Delegate rendering to the image drawable
     */
    override fun onDraw(canvas: Canvas) {
        ViewUtils.onDraw(this, canvas) {
            super.onDraw(canvas)
            imageDrawable.draw(canvas)
        }
    }

    override fun verifyDrawable(dr: Drawable): Boolean {
        if (dr === imageDrawable) {
            return true
        }
        return ViewUtils.verifyDrawable(this, dr) || super.verifyDrawable(dr)
    }

    // Android View overrides

    override fun onMeasure(widthMeasureSpec: Int, heightMeasureSpec: Int) {
        val width = View.MeasureSpec.getSize(widthMeasureSpec)
        val widthMode = View.MeasureSpec.getMode(widthMeasureSpec)
        val height = View.MeasureSpec.getSize(heightMeasureSpec)
        val heightMode = View.MeasureSpec.getMode(heightMeasureSpec)

        val measuredWidth: Int
        val measuredHeight: Int

        if (widthMode == View.MeasureSpec.EXACTLY && heightMode == View.MeasureSpec.EXACTLY) {
            measuredWidth = width
            measuredHeight = height
            setMeasuredDimension(measuredWidth, measuredHeight)
            return
        }

        val asset = this.currentAsset
        if (asset != null) {
            val maxWidth = if (widthMode == View.MeasureSpec.UNSPECIFIED) -1.0 else coordinateResolver.fromPixel(width)
            val maxHeight = if (heightMode == View.MeasureSpec.UNSPECIFIED) -1.0 else coordinateResolver.fromPixel(height)
            measuredWidth = coordinateResolver.toPixel(asset.measureWidth(maxWidth, maxHeight))
            measuredHeight = coordinateResolver.toPixel(asset.measureHeight(maxWidth, maxHeight))
        } else {
            measuredWidth = 0
            measuredHeight = 0
        }
        setMeasuredDimension(measuredWidth, measuredHeight)
    }

    override fun hasOverlappingRendering(): Boolean {
        return false
    }

    override fun dispatchDraw(canvas: Canvas) {
        ViewUtils.dispatchDraw(this, canvas) {
            super.dispatchDraw(canvas)
        }
    }

    override fun onLayout(changed: Boolean, left: Int, top: Int, right: Int, bottom: Int) {
        super.onLayout(changed, left, top, right, bottom)

        imageDrawable.setBounds(0, 0, width, height)
        imageDrawable.onLayoutCompleted()
    }

    /**
     * Setters for the image drawable
     */

    fun setPlaceholder(placeholder: Drawable?) {
        imageDrawable.setPlaceholder(placeholder)
    }

    fun setScaleType(scaleType: ImageView.ScaleType) {
        imageDrawable.scaleType = scaleType
    }

    fun setTint(tint: Int) {
        imageDrawable.setTint(tint)
    }

    fun setContentScaleX(contentScaleX: Float) {
        imageDrawable.contentScaleX = contentScaleX
    }

    fun setContentScaleY(contentScaleY: Float) {
        imageDrawable.contentScaleY = contentScaleY
    }

    fun setContentRotation(contentRotation: Float) {
        imageDrawable.contentRotation = contentRotation
    }
}
