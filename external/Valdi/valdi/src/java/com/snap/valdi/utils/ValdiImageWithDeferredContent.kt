package com.snap.valdi.utils

import com.snap.valdi.exceptions.ValdiException

abstract class ValdiImageWithDeferredContent<T>(contentProvider: T): ValdiImage() {

    private var contentProvider: T? = contentProvider
    private var resolvedContent: ValdiImageContent? = null

    protected abstract fun onGetContent(contentProvider: T): ValdiImageContent
    protected open fun onDestroyContent(contentProvider: T) {}

    final override fun getContent(): ValdiImageContent {
        return synchronized(this) {
            var content = this.resolvedContent

            if (content == null) {
                val contentProvider = this.contentProvider
                        ?: throw ValdiException("Content was destroyed")
                content = onGetContent(contentProvider)
                this.resolvedContent = content
            }

            content
        }
    }

    override fun onDestroyBitmap() {
        synchronized(this) {
            val contentProvider = this.contentProvider
            this.contentProvider = null
            this.resolvedContent = null

            if (contentProvider != null) {
                onDestroyContent(contentProvider)
            }
        }
    }
}
