package com.snap.valdi.views

/**
 * Any view that conforms to this interface will be considered as recyclable, meaning
 * that it can be enqueued into a view pool and re-used by different components.
 */
interface ValdiRecyclableView {

    /**
     * Prepare the view for recycling.
     * Attributes exposed to Valdi don't need to be reset as they
     * will be reset automatically.
     */
    fun prepareForRecycling() {}

}
