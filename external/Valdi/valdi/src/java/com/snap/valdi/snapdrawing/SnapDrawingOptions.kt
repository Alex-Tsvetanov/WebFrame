package com.snap.valdi.snapdrawing

data class SnapDrawingOptions(
    val renderMode: SnapDrawingRenderMode = SnapDrawingRenderMode.TEXTURE_VIEW,
    val enableSynchronousDraw: Boolean = false,
    val useAndroidStyleScroller: Boolean = false,
    val surfaceViewZOrder: SnapDrawingSurfaceViewZOrder = SnapDrawingSurfaceViewZOrder.DEFAULT,
)