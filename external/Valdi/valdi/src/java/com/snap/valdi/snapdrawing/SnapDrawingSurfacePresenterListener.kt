package com.snap.valdi.snapdrawing

import android.graphics.Canvas
import android.view.Surface

interface SnapDrawingSurfacePresenterListener {

    fun drawPresenterInCanvas(width: Int, height: Int, surfacePresenterId: Int, canvas: Canvas): Boolean

    fun onPresenterNeedsRedraw(surfacePresenterId: Int)

    fun onPresenterHasNewSurface(surfacePresenterId: Int, surface: Surface?)

    fun onPresenterSurfaceSizeChanged(surfacePresenterId: Int)

}
