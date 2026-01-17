package com.snap.valdi.utils

interface ValdiImageLoadCompletion {

    fun onImageLoadComplete(imageWidth: Int, imageHeight: Int, image: ValdiImage?, error: Throwable?)

}
