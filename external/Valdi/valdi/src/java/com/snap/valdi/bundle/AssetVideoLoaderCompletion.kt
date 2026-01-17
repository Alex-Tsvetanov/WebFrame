package com.snap.valdi.bundle

import com.snap.valdi.exceptions.ValdiFatalException
import com.snap.valdi.exceptions.messageWithCauses
import com.snap.valdi.utils.ValdiVideoPlayerCreatedCompletion
import com.snap.valdi.utils.NativeRef
import com.snap.valdi.utils.ValdiVideoPlayer
import com.snapchat.client.valdi.NativeBridge

class AssetVideoLoaderCompletion(handle: Long): NativeRef(handle), ValdiVideoPlayerCreatedCompletion {

    override fun onVideoPlayerCreated(player: ValdiVideoPlayer, error: Throwable?) {
        if (player != null) {
            NativeBridge.notifyAssetLoaderCompleted(nativeHandle, player, null, /* isImage */ false)
        } else if (error != null) {
            NativeBridge.notifyAssetLoaderCompleted(nativeHandle, null, error.messageWithCauses(), /* isImage */ false)
        } else {
            throw ValdiFatalException("VideoLoadCompletion should submit either a player or an error")
        }

        // Dispose as soon as the first onImageLoadComplete is called, so that we can destroy the
        // native completion handler potentially earlier.
        dispose()
    }

}