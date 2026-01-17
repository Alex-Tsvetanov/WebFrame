package com.snap.valdi.snapdrawing

import androidx.annotation.Keep
import com.snap.valdi.ViewRef

@Keep
interface SurfacePresenterManager {

    @Keep
    fun createPresenterWithDrawableSurface(surfacePresenterId: Int, zIndex: Int)

    @Keep
    fun createPresenterForEmbeddedView(surfacePresenterId: Int, zIndex: Int, viewToEmbed: ViewRef)

    @Keep
    fun setPresenterZIndex(surfacePresenterId: Int, zIndex: Int)

    @Keep
    fun removePresenter(surfacePresenterId: Int)

    @Keep
    fun setEmbeddedViewPresenterState(surfacePresenterId: Int,
                                      left: Int,
                                      top: Int,
                                      right: Int,
                                      bottom: Int,
                                      opacity: Float,
                                      transform: Any?,
                                      transformChanged: Boolean,
                                      clipPath: Any?,
                                      clipPathChanged: Boolean)

    @Keep
    fun onDrawableSurfacePresenterUpdated(surfacePresenterId: Int)

}
