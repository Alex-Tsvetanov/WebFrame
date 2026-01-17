package com.snap.valdi.utils

interface IScheduler {
    fun schedule(work: () -> Unit)
}
