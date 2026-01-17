package com.snap.valdi.utils

interface IAsyncWorkScheduler {
    fun scheduleWork(work: (onComplete: () -> Unit) -> Unit): Disposable
}