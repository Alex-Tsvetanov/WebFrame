package com.snap.valdi.utils

class ValdiImageWithContent(private val content: ValdiImageContent): ValdiImage() {

    override fun getContent(): ValdiImageContent {
        return content
    }

    override fun onDestroyBitmap() {}
}