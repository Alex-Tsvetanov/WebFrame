@file:JvmName("MainThreadUtils")

package com.snap.valdi.utils

import android.os.Handler
import android.os.Looper

// Convenience methods to run runnables in the UI thread.

fun runOnMainThreadIfNeeded(task: () -> Unit) {
    if (Thread.currentThread() == Looper.getMainLooper().thread) {
        task()
    } else {
        dispatchOnMainThread(task)
    }
}

fun dispatchOnMainThread(task: () -> Unit) {
    handler.post(task)
}

fun isMainThread(): Boolean {
    return Thread.currentThread() === Looper.getMainLooper().thread
}

fun assertMainThread() {
    if (!isMainThread()) {
        throw RuntimeException("This action can only be performed from the main thread")
    }
}

fun assertNotMainThread() {
    if (isMainThread()) {
        throw RuntimeException("This action should never be performed from the main thread")
    }
}

fun runOnMainThreadDelayed(delayMs: Long, task: () -> Unit) {
    handler.postDelayed(task, delayMs)
}

fun runOnMainThreadDelayed(delayMs: Long, task: Runnable) {
    handler.postDelayed(task, delayMs)
}

fun getValdiHandler(): Handler = handler

private val handler = object: Handler(Looper.getMainLooper()) {}
