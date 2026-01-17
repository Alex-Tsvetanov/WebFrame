package com.snap.valdi

import com.snap.valdi.context.ValdiContext
import com.snap.valdi.snapdrawing.SnapDrawingOptions

class ValdiContextConfiguration(
        val renderBackend: RenderBackend? = null,
        var snapDrawingOptions: SnapDrawingOptions? = null,
        val useLegacyMeasureBehavior: Boolean? = null,
        val preloadingMode: PreloadingMode? = null,
        /**
         * Callback which will be called right after the ValdiContext was created,
         * and before onCreate() is dispatched.
         */
        val prepareContext: ((ValdiContext) -> Unit)? = null
)