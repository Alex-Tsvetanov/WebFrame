package com.snap.valdi.snapdrawing

/**
 * Specifies the ZOrder set on the underlying SurfaceView objects.
 */
enum class SnapDrawingSurfaceViewZOrder {
    /**
     * Default Android's Z order for a newly created SurfaceView
     */
    DEFAULT,

    /**
     * See https://developer.android.com/reference/android/view/SurfaceView#setZOrderMediaOverlay(boolean)
     */
    MEDIA_OVERLAY,

    /**
     * See https://developer.android.com/reference/android/view/SurfaceView#setZOrderOnTop(boolean)
     */
    ON_TOP,
}