package com.snap.valdi

enum class RenderBackend {
    /**
     * Let the runtime decides what RenderBackend to use.
     */
    DEFAULT,

    /**
     * Always use the Android Render backend for rendering the UI
     * of Valdi components.
     */
    ANDROID,

    /**
     * Always use the SnapDrawing Render backend for rendering the UI
     * of Valdi components.
     */
    SNAP_DRAWING,
    ;
}