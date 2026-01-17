package com.snap.valdi.utils.executors

import com.snap.valdi.utils.Disposable
import java.util.concurrent.Executor

interface ExecutorFactory {
    fun makeExecutor(name: String, threadPriority: Int, onTaskAdded: ((task: Disposable) -> Unit)?): Executor
}