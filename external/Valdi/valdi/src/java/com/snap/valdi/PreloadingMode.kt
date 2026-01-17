package com.snap.valdi

enum class PreloadingMode {
    /**
     * Preload as much work within the runtime in parallel while
     * the feature is being created. This limits the chance of
     * blocking the main thread, but increases contention.
     * This is the default behavior.
     */
    AGGRESSIVE,

    /**
     * Reduce preloading as much as possible within the runtime.
     * This is suited when rendering a feature on app startup,
     * where contention is typically already very high, and the
     * main thread is not usable until the feature launches.
     */
    DISABLED,
}