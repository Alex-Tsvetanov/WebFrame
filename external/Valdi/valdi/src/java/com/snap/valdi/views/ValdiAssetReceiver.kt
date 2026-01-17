package com.snap.valdi.views

/**
 * The AssetReceiver interface represents a View that can receive loaded assets
 * once they've been loaded from an AssetLoader. Views that can handle loaded asset
 * should implement this interface.
 */
interface ValdiAssetReceiver {

    /**
     * Called when an asset has been fully loaded. The asset instance will be the object that
     * was returned from the AssetLoader implementation.
     * shouldFlip will be true if "flipOnRtl" was set to true on the element, and that the node
     * is in RTL context.
     */
    fun onAssetChanged(asset: Any?, shouldFlip: Boolean)

}