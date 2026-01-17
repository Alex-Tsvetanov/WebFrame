package com.snap.valdi.utils

public inline fun <T, reified R> List<T>.arrayMap(transform: (T) -> R): Array<R> {
    return Array<R>(size) {
        transform(this[it])
    }
}

public inline fun <T, reified R> Array<T>.arrayMap(transform: (T) -> R): Array<R> {
    return Array<R>(size) {
        transform(this[it])
    }
}