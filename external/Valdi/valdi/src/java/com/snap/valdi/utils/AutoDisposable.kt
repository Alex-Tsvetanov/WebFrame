package com.snap.valdi.utils

import androidx.annotation.Keep
import java.util.concurrent.atomic.AtomicBoolean
import java.util.concurrent.atomic.AtomicInteger

/**
 * AutoDisposable is an abstract class that can be used to return a Java value to C++
 * that should be automatically disposed whenever C++ does not need the value anymore.
 */
@Keep
abstract class AutoDisposable: Disposable {

    private val retainCount = AtomicInteger()
    private var disposed = AtomicBoolean()

    @Keep
    fun retain() {
        retainCount.incrementAndGet()
    }

    @Keep
    fun release() {
        val count = retainCount.decrementAndGet()
        if (count == 0) {
            val wasDisposed = disposed.getAndSet(true)
            if (!wasDisposed) {
                dispose()
            }
        }
    }

}
