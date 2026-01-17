package com.snap.valdi

import java.lang.Runnable

interface MainThreadBatchDispatcher {

    fun executeMainThreadBatch(runnable: Runnable)

}