package com.snap.valdi.snapdrawing

import android.content.Context
import android.view.View
import com.snap.valdi.ViewRef
import com.snap.valdi.extensions.removeFromParentView
import com.snap.valdi.utils.BitmapPool
import com.snap.valdi.utils.runOnMainThreadIfNeeded
import com.snap.valdi.views.snapdrawing.SnapDrawingEmbeddedView
import com.snap.valdi.views.snapdrawing.SnapDrawingSurfaceView
import com.snap.valdi.views.snapdrawing.SnapDrawingTextureView

class SurfacePresenterFactory(val bitmapPool: BitmapPool,
                              val context: Context) {
    private val viewByPresenterId = hashMapOf<Int, View>()
    private val embeddedViewsPool = mutableListOf<SnapDrawingEmbeddedView>()
    private val textureViewsPool = mutableListOf<SnapDrawingTextureView>()
    private val surfaceViewsPool = mutableListOf<SnapDrawingSurfaceView>()

    private inline fun <T: View> createViewFromPool(pool: MutableList<T>, factory: (context: Context) -> T): T {
        if (pool.isEmpty()) {
            return factory(context)
        }
        return pool.removeAt(pool.lastIndex)
    }

    private fun <T> enqueueViewToPool(pool: MutableList<T>, view: T) {
        pool.add(view)
    }

    private fun createSurfaceView(zOrder: SnapDrawingSurfaceViewZOrder): SnapDrawingSurfaceView {
        val surfaceView = createViewFromPool(surfaceViewsPool) { SnapDrawingSurfaceView(it) }

        when (zOrder) {
            SnapDrawingSurfaceViewZOrder.DEFAULT -> {
                surfaceView.setZOrderOnTop(false)
                surfaceView.setZOrderMediaOverlay(false)
            }
            SnapDrawingSurfaceViewZOrder.MEDIA_OVERLAY -> {
                surfaceView.setZOrderMediaOverlay(true)
            }
            SnapDrawingSurfaceViewZOrder.ON_TOP -> {
                surfaceView.setZOrderOnTop(true)
            }
        }

        return surfaceView
    }

    private fun createTextureView(): SnapDrawingTextureView {
        return createViewFromPool(textureViewsPool) { SnapDrawingTextureView(it) }
    }

    private fun createEmbeddedView(): SnapDrawingEmbeddedView {
        return createViewFromPool(embeddedViewsPool) { SnapDrawingEmbeddedView(it) }
    }

    private fun setupPresenter(surfacePresenterId: Int,
                               presenter: SnapDrawingSurfacePresenter,
                               listener: SnapDrawingSurfacePresenterListener) {
        val presenterView = presenter as View
        viewByPresenterId[surfacePresenterId] = presenterView

        presenter.setup(surfacePresenterId, listener)
    }

    fun createPresenterWithDrawableSurface(renderMode: SnapDrawingRenderMode,
                                           surfacePresenterId: Int,
                                           surfaceViewZOrder: SnapDrawingSurfaceViewZOrder,
                                           listener: SnapDrawingSurfacePresenterListener): View {
        val presenter: SnapDrawingSurfacePresenter = when (renderMode) {
            SnapDrawingRenderMode.SURFACE_VIEW -> {
                createSurfaceView(surfaceViewZOrder)
            }

            SnapDrawingRenderMode.TEXTURE_VIEW -> {
                createTextureView()
            }
        }

        setupPresenter(surfacePresenterId, presenter, listener)

        return presenter as View
    }

    fun createPresenterForEmbeddedView(surfacePresenterId: Int,
                                       viewToEmbed: ViewRef,
                                       listener: SnapDrawingSurfacePresenterListener): View {
        val view = createEmbeddedView()
        view.innerView = viewToEmbed.get()

        setupPresenter(surfacePresenterId, view, listener)

        return view
    }

    fun getPresenterForId(surfacePresenterId: Int): View {
        return viewByPresenterId[surfacePresenterId]!!
    }

    fun clearCache() {
        embeddedViewsPool.clear()
        textureViewsPool.clear()
        surfaceViewsPool.clear()
    }

    fun removePresenter(surfacePresenterId: Int) {
        val view = viewByPresenterId.remove(surfacePresenterId)!!

        (view as SnapDrawingSurfacePresenter).teardown()

        view.removeFromParentView()

        when (view) {
            is SnapDrawingSurfaceView -> {
                enqueueViewToPool(surfaceViewsPool, view)
            }
            is SnapDrawingTextureView -> {
                // Texture can fail to render properly if their size doesn't change
                // when removing them from the view hierarchy and adding them back later.
                view.layout(0, 0, 0, 0)
                enqueueViewToPool(textureViewsPool, view)
            }
            is SnapDrawingEmbeddedView -> {
                enqueueViewToPool(embeddedViewsPool, view)
            }
        }
    }

    fun setEmbeddedViewPresenterState(surfacePresenterId: Int,
                                      left: Int,
                                      top: Int,
                                      right: Int,
                                      bottom: Int,
                                      opacity: Float,
                                      transform: Any?,
                                      transformChanged: Boolean,
                                      clipPath: Any?,
                                      clipPathChanged: Boolean) {
        val embeddedView = getPresenterForId(surfacePresenterId) as SnapDrawingEmbeddedView
        embeddedView.setState(left,
                top,
                right,
                bottom,
                opacity,
                transform as? FloatArray,
                transformChanged,
                clipPath as? FloatArray,
                clipPathChanged
        )
    }

    private fun invalidatePresenterIfNeeded(surfacePresenterId: Int) {
        val presenter = viewByPresenterId[surfacePresenterId] as? SnapDrawingTextureView ?: return
        presenter.forceInvalidate()
    }

    fun onDrawableSurfacePresenterUpdated(surfacePresenterId: Int) {
        runOnMainThreadIfNeeded {
            invalidatePresenterIfNeeded(surfacePresenterId)
        }
    }
}