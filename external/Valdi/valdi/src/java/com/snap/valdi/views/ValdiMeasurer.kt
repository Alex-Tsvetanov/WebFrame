package com.snap.valdi.views

interface ValdiMeasurer {

    /**
     * Returns whether a placeholder view should be used to calculate the layout. Otherwise, the view instance
     * attached to the flexbox node will be used.
     * Default is true.
     */
    fun canUsePlaceholderViewToMeasure(): Boolean

}