package com.snap.valdi.snapdrawing

import android.view.MotionEvent
import android.view.Surface
import com.snap.valdi.ViewRef
import com.snap.valdi.utils.BitmapHandler
import com.snap.valdi.utils.NativeRef
import com.snapchat.client.valdi.NativeBridge

class SnapDrawingRootHandle(handle: Long, private val displayScale: Float): NativeRef(handle) {

    enum class TouchEventType {
        DOWN,
        MOVED,
        IDLE,
        UP,
        WHEEL,
        CANCELLED,
        POINTER_UP,
        POINTER_DOWN;

        fun toMotionEventAction(): Int {
            return when (this) {
                DOWN -> MotionEvent.ACTION_DOWN
                MOVED -> MotionEvent.ACTION_MOVE
                IDLE -> MotionEvent.ACTION_MOVE
                UP -> MotionEvent.ACTION_UP
                WHEEL -> MotionEvent.ACTION_SCROLL
                CANCELLED -> MotionEvent.ACTION_CANCEL
                POINTER_UP -> MotionEvent.ACTION_POINTER_UP
                POINTER_DOWN -> MotionEvent.ACTION_POINTER_DOWN
            }
        }

        companion object {
            @JvmStatic
            fun fromMotionEventAction(motionEventAction: Int): TouchEventType? {
                return when (motionEventAction) {
                    MotionEvent.ACTION_DOWN -> DOWN
                    MotionEvent.ACTION_MOVE -> MOVED
                    MotionEvent.ACTION_UP -> UP
                    MotionEvent.ACTION_SCROLL -> WHEEL
                    MotionEvent.ACTION_CANCEL -> CANCELLED
                    MotionEvent.ACTION_POINTER_UP -> POINTER_UP
                    MotionEvent.ACTION_POINTER_DOWN -> POINTER_DOWN
                    else -> null
                }
            }

            fun fromRawInt(rawInt: Int): TouchEventType? {
                return when (rawInt) {
                    0 -> DOWN
                    1 -> MOVED
                    2 -> IDLE
                    3 -> UP
                    4 -> WHEEL
                    5 -> CANCELLED
                    6 -> POINTER_UP
                    7 -> POINTER_DOWN
                    else -> null
                }
            }

            @JvmStatic
            inline fun convertAndDispatchTouchEvent(event: MotionEvent,
                                                    receiver: (type: Int, eventTimeNanos: Long, centerX: Int, centerY: Int, directionX: Int, directionY: Int, pointerCount: Int, actionIndex: Int, pointerLocations: IntArray) -> Boolean): Boolean {
                val type = TouchEventType.fromMotionEventAction(event.actionMasked) ?: return false
                // For some reason the nanoseconds API is hidden, we can't be precise here
                val eventTimeNanos = event.eventTime * 1000000

                val refX = event.x.toInt()
                val refY = event.y.toInt()
                var centerX = refX
                var centerY = refY
                var directionX = 0
                var directionY = 0

                val pointerCount = event.getPointerCount()
                if (pointerCount > 0) {
                    var sumX = 0
                    var sumY = 0
                    for (index in 0 until pointerCount) {
                        sumX += event.getX(index).toInt()
                        sumY += event.getY(index).toInt()
                    }
                    centerX = sumX / pointerCount
                    centerY = sumY / pointerCount
                    directionX = centerX - refX
                    directionY = centerY - refY
                }
                val actionIndex = event.actionIndex
                val pointerLocations = IntArray(pointerCount * 2).apply {
                    var i = 0
                    var pointerIndex = 0
                    while (i < this.size) {
                        this[i++] = event.getX(pointerIndex).toInt()
                        this[i++] = event.getY(pointerIndex).toInt()
                        pointerIndex++
                    }
                }

                return receiver(type.ordinal, eventTimeNanos, centerX, centerY, directionX, directionY, pointerCount, actionIndex, pointerLocations)
            }
        }
    }

    fun dispatchTouchEvent(event: MotionEvent): Boolean {
        return TouchEventType.convertAndDispatchTouchEvent(event) { type, eventTimeNanos, centerX, centerY, directionX, directionY, pointerCount, actionIndex, pointerLocations ->
            NativeBridge.dispatchSnapDrawingTouchEvent(nativeHandle,
                    type,
                    eventTimeNanos,
                    centerX,
                    centerY,
                    directionX,
                    directionY,
                    pointerCount,
                    actionIndex,
                    pointerLocations,
                    MotionEvent.obtain(event) /* TODO(simon): Don't leak out MotionEvent */
            )
        }
    }

    fun drawInBitmap(bitmap: BitmapHandler, surfacePresenterId: Int, isOwned: Boolean) {
        NativeBridge.snapDrawingDrawInBitmap(nativeHandle, surfacePresenterId, bitmap, isOwned)
    }

    fun layout(width: Int, height: Int) {
        NativeBridge.snapDrawingLayout(nativeHandle, width, height)
    }

    fun setSurface(surfacePresenterId: Int, surface: Surface?) {
        NativeBridge.snapDrawingSetSurface(nativeHandle, surfacePresenterId, surface)
    }

    fun setSurfaceNeedsRedraw(surfacePresenterId: Int) {
        NativeBridge.snapDrawingSetSurfaceNeedsRedraw(nativeHandle, surfacePresenterId)
    }

    fun onSurfaceSizeChanged(surfacePresenterId: Int) {
        NativeBridge.snapDrawingOnSurfaceSizeChanged(nativeHandle, surfacePresenterId)
    }

    fun setSurfacePresenterManager(surfacePresenterManager: SurfacePresenterManager?) {
        NativeBridge.snapDrawingSetSurfacePresenterManager(nativeHandle, surfacePresenterManager)
    }

    fun convertUnit(value: Int): Float {
        return value.toFloat() / displayScale
    }
}
