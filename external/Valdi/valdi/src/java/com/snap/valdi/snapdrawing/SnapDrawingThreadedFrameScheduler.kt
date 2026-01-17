package com.snap.valdi.snapdrawing

import android.os.Build
import android.view.Choreographer
import androidx.annotation.RequiresApi
import java.util.concurrent.ExecutorService
import java.util.concurrent.Executors
import java.util.concurrent.ThreadFactory

/**
 * An implementation of SnapDrawingFrameScheduler that uses an executor service to schedule frames.
 */
@RequiresApi(Build.VERSION_CODES.JELLY_BEAN)
class SnapDrawingThreadedFrameScheduler: SnapDrawingFrameScheduler() {

    private var executor: ExecutorService? = null

    override fun onStart(threadFactory: ThreadFactory) {
        executor = Executors.newSingleThreadExecutor(threadFactory)
    }

    override fun onStop() {
        val executor = this.executor
        this.executor = null
        executor?.shutdownNow()
    }

    override fun schedule(callback: Choreographer.FrameCallback) {
        executor?.execute {
            callback.doFrame(System.nanoTime())
        }
    }
}