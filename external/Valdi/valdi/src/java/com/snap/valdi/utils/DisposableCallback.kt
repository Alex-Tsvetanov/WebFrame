package com.snap.valdi.utils

/**
 * Base class that holds a callback that can be disposed. Subclasses
 * should call extractCallback() whenever they need to call it. The returned
 * callback will be null if the object was disposed or if the callback was
 * already extracted.
 */
open class DisposableCallback<T>(private var callback: T?): Disposable {

    protected fun extractCallback(): T? {
        return synchronized(this) {
            val callback = this.callback
            this.callback = null
            callback
        }
    }

    override fun dispose() {
        extractCallback()
    }
}