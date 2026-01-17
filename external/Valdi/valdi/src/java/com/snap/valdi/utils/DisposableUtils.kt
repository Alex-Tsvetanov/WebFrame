package com.snap.valdi.utils

object DisposableUtils {

    @JvmStatic
    fun disposeAny(obj: Any?): Int {
        return when (obj) {
            null -> 0
            is Disposable -> {
                obj.dispose()
                1
            }
            is DisposablePrivate -> {
                obj.__dispose__()
                1
            }
            is List<*> -> {
                var disposedItems = 0
                for (value in obj) {
                    disposedItems += disposeAny(value)
                }
                disposedItems
            }
            is Map<*, *> -> {
                var disposedItems = 0
                for (keyValue in obj) {
                    disposedItems += disposeAny(keyValue.key)
                    disposedItems += disposeAny(keyValue.value)
                }
                disposedItems
            }
            else -> 0
        }
    }
}