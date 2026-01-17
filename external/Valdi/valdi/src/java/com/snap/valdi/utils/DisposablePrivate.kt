package com.snap.valdi.utils

/**
 * Same as Disposable but with a somewhat mangled name
 * so that it doesn't conflict with user provided property names.
 */
interface DisposablePrivate {
    fun __dispose__()
}