package com.snap.valdi.utils

import android.net.Uri
import com.snap.valdi.exceptions.ValdiException

/**
 * A ValdiImageLoader implementation is responsible to load Image Assets.
 * Image assets can be requested as Android Bitmap, meant to be consumed by Android Java code,
 * or as raw content, meant to be consumed by SnapDrawing at the native level.
 */
interface ValdiImageLoader: ValdiAssetLoader {

    /**
    Load the bitmap for the given URL and call the completion when done.
     */
    fun loadImage(requetPayload: Any,
                  options: ValdiImageLoadOptions,
                  completion: ValdiImageLoadCompletion): Disposable?

}
