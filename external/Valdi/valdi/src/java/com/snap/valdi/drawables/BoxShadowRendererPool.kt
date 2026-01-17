package com.snap.valdi.drawables

import android.content.Context
import android.graphics.Bitmap
import android.graphics.Rect
import android.os.Build
import androidx.annotation.RequiresApi
import com.snap.valdi.drawables.utils.BorderRadii
import com.snap.valdi.logger.Logger
import com.snap.valdi.utils.BitmapPool
import com.snap.valdi.utils.trace
import java.lang.ref.WeakReference

class BoxShadowRendererPool(context: Context, logger: Logger) {

    val bitmapPool = BitmapPool(context, Bitmap.Config.ALPHA_8, logger)

    private val pooledRenderers = mutableListOf<WeakReference<BoxShadowRendererImpl>>()

    @RequiresApi(Build.VERSION_CODES.LOLLIPOP)
    internal fun createImpl(drawBounds: Rect,
                            borderRadii: BorderRadii?,
                            widthOffset: Int,
                            heightOffset: Int,
                            blurAmount: Float): BoxShadowRendererImpl {
        var availableRenderer: BoxShadowRendererImpl? = null
        val it = pooledRenderers.iterator()
        while (it.hasNext()) {
            val ref = it.next()
            val renderer = ref.get()
            if (renderer == null) {
                it.remove()
                continue
            }

            if (!renderer.needsConfigure(drawBounds, borderRadii, widthOffset, heightOffset, blurAmount)) {
                renderer.useCount++
                return renderer
            }

            if (renderer.useCount == 0) {
                availableRenderer = renderer
            }
        }

        if (availableRenderer == null) {
            availableRenderer = BoxShadowRendererImpl(bitmapPool)
            pooledRenderers.add(WeakReference(availableRenderer))
        }

        trace({"Valdi.configureBoxShadow"}) {
            availableRenderer.configure(drawBounds, borderRadii, widthOffset, heightOffset, blurAmount)
        }

        availableRenderer.useCount++
        return availableRenderer
    }

    internal fun releaseImpl(impl: BoxShadowRendererImpl) {
        impl.useCount--
    }

    fun create(): BoxShadowRenderer {
        return DeferredBoxShadowRenderer(this)
    }
}