package com.snap.valdi.snapdrawing

import android.os.Build
import android.os.Handler
import android.os.Looper
import android.view.Choreographer
import androidx.annotation.Keep
import androidx.annotation.RequiresApi
import com.snap.valdi.utils.NativeRef
import com.snapchat.client.valdi.NativeBridge
import java.util.concurrent.ThreadFactory

@RequiresApi(Build.VERSION_CODES.JELLY_BEAN)
abstract class SnapDrawingFrameScheduler {

    private class CallbackHandle(handle: Long): NativeRef(handle), Choreographer.FrameCallback {
        override fun doFrame(frameTimeNanos: Long) {
            NativeBridge.snapDrawingCallFrameCallback(this.nativeHandle, frameTimeNanos)
            destroy()
        }
    }

    private var mainThreadHandler: Handler = Handler(Looper.getMainLooper())
    private var started = false

    protected fun postCallbackOnHandler(handle: Choreographer.FrameCallback, handler: Handler) {
        if (Looper.myLooper() !== handler.looper) {
            handler.post {
                postCallbackOnHandler(handle, handler)
            }
            return
        }

        Choreographer.getInstance().postFrameCallback(handle)
    }

    protected abstract fun onStart(threadFactory: ThreadFactory)
    protected abstract fun onStop()
    protected abstract fun schedule(callback: Choreographer.FrameCallback)

    @Keep
    fun stop() {
        synchronized(this) {
            if (this.started) {
                this.started = false
                onStop()
            }
        }
    }

    @Keep
    fun onNextVSync(handle: Long) {
        val callback = CallbackHandle(handle)

        synchronized(this) {
            if (!this.started) {
                this.started = true
                onStart(createThreadFactory())
            }
            schedule(callback)
        }
    }

    @Keep
    fun onMainThread(handle: Long) {
        postCallbackOnHandler(CallbackHandle(handle), this.mainThreadHandler)
    }

    companion object {
        @JvmStatic
        private fun createThreadFactory(): ThreadFactory {
            return ThreadFactory { r ->
                Thread(r, "SnapDrawing RenderThread").apply {
                    priority = Thread.MAX_PRIORITY - 1
                }
            }
        }
    }

}
