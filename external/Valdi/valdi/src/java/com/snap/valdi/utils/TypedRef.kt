package com.snap.valdi.utils

import java.lang.ref.WeakReference

open class TypedRef<T>(item: T, strong: Boolean): Ref {

    private val weakReference = WeakReference(item)
    private var strongReference: T?

    init {
        strongReference = if (strong) item else null
    }

    fun makeStrong() {
        strongReference = weakReference.get()
    }

    fun makeWeak() {
        strongReference = null
    }

    override fun get(): T? {
        return strongReference ?: weakReference.get()
    }

}