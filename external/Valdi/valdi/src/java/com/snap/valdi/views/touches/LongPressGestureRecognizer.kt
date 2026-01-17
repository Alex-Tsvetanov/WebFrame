package com.snap.valdi.views.touches

import android.view.MotionEvent
import android.view.View
import android.view.ViewConfiguration
import com.snap.valdi.utils.Disposable
import com.snap.valdi.utils.DisposableRunnable
import com.snap.valdi.utils.runOnMainThreadDelayed

interface LongPressGestureRecognizerListener {
    fun onRecognized(gesture: LongPressGestureRecognizer, state: ValdiGestureRecognizerState, x: Int, y: Int, pointerCount: Int, pointerLocations: List<ValdiGesturePointer>)
    fun shouldBegin(gesture: LongPressGestureRecognizer, x: Int, y: Int, pointerCount: Int, pointerLocations: List<ValdiGesturePointer>): Boolean
}

open class LongPressGestureRecognizer(
        view: View,
        val listener: LongPressGestureRecognizerListener
) : AndroidDetectorGestureRecognizer(view, false) {

    var allowSelfTrigger = false

    private var disposable: Disposable? = null

    var longPressDurationMs: Long = ViewConfiguration.getLongPressTimeout().toLong()

    private fun onLongPress() {
        if (destroyed || state != ValdiGestureRecognizerState.POSSIBLE || !allowSelfTrigger) {
            return
        }
        updateState(ValdiGestureRecognizerState.BEGAN)
    }

    private fun clearRunnable() {
        disposable?.dispose()
        disposable = null
    }

    override fun onUpdate(event: MotionEvent) {
        if (state == ValdiGestureRecognizerState.POSSIBLE) {
            allowSelfTrigger = true
            gestureDetector.onTouchEvent(event)

            if (event.actionMasked == MotionEvent.ACTION_DOWN && state == ValdiGestureRecognizerState.POSSIBLE) {
                val cb = DisposableRunnable {
                    onLongPress()
                }
                clearRunnable()
                disposable = cb

                runOnMainThreadDelayed(longPressDurationMs, cb)
            }
        }
    }

    override fun onProcess() {
        if (state == ValdiGestureRecognizerState.BEGAN) {
            listener.onRecognized(this, state, x, y, pointerCount, pointerLocations)
        }
    }

    override fun shouldBegin(): Boolean {
        return listener.shouldBegin(this, x, y, pointerCount, pointerLocations)
    }

    override fun onReset(event: MotionEvent) {
        super.onReset(event)

        clearRunnable()
        allowSelfTrigger = false
    }

}