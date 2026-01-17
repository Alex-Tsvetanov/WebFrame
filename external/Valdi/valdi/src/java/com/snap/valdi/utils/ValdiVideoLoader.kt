package com.snap.valdi.utils

import android.net.Uri
import com.snap.valdi.exceptions.ValdiException

interface ValdiVideoLoader: ValdiAssetLoader {

    /**
    Load the video for the given URL and call the completion when done.
     */
    fun loadVideo(requestPayload: Any,
                  completion: ValdiVideoPlayerCreatedCompletion): Disposable?

    override fun getSupportedOutputTypes(): Int {
        return ValdiAssetLoadOutputType.VIDEO.value
    }

}
