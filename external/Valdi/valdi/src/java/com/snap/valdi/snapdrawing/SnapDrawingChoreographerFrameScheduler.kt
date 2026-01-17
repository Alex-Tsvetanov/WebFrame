package com.snap.valdi.snapdrawing

import android.os.Build
import android.os.Handler
import android.os.Looper
import android.view.Choreographer
import androidx.annotation.RequiresApi
import java.util.concurrent.CountDownLatch
import java.util.concurrent.ThreadFactory

/**
 * An implementation of SnapDrawingFrameScheduler that uses Choreographer to schedule frames.
 */
@RequiresApi(Build.VERSION_CODES.JELLY_BEAN)
class SnapDrawingChoreographerFrameScheduler: SnapDrawingFrameScheduler() {

    private var thread: Thread? = null
    private var looper: Looper? = null
    private var handler: Handler? = null

    override fun onStart(threadFactory: ThreadFactory) {
        val latch = CountDownLatch(1)

        val thread = threadFactory.newThread {
            doLoop(latch)
        }

        this.thread = thread

        thread.start()

        latch.await()
    }

    override fun onStop() {
        val looper = this.handler?.looper
        val thread = this.thread

        this.thread = null
        this.handler = null

        looper?.quit()
        thread?.join()
    }

    override fun schedule(callback: Choreographer.FrameCallback) {
        this.handler?.let {
            postCallbackOnHandler(callback, it)
        }
    }

    private fun doLoop(latch: CountDownLatch) {
        Looper.prepare()
        val looper = Looper.myLooper()!!
        val handler = Handler(looper)

        this.looper = looper
        this.handler = handler

        latch.countDown()

        Looper.loop()
    }

}