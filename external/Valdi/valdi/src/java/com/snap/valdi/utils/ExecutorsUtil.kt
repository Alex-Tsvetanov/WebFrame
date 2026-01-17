package com.snap.valdi.utils

import java.util.concurrent.ExecutorService
import java.util.concurrent.Executors
import java.util.concurrent.LinkedBlockingQueue
import java.util.concurrent.ThreadFactory
import java.util.concurrent.ThreadPoolExecutor
import java.util.concurrent.TimeUnit

object ExecutorsUtil {
    @JvmStatic
    fun newSingleThreadCachedExecutor(threadFactory: ThreadFactory? = Executors.defaultThreadFactory()): ExecutorService {
        return ThreadPoolExecutor(0,
                1,
                60L,
                TimeUnit.SECONDS,
                LinkedBlockingQueue(),
                threadFactory)
    }
}
