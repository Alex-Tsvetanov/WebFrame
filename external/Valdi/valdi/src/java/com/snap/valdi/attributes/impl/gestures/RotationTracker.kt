package com.snap.valdi.attributes.impl.gestures

import android.view.MotionEvent

class RotationTracker {
    private var pointerId1: Int? = null
    private var pointerId2: Int? = null
    private var pointer1StartX = 0f
    private var pointer1StartY = 0f
    private var pointer2StartX = 0f
    private var pointer2StartY = 0f

    var rotation = 0f
        private set

    fun onTouchEvent(event: MotionEvent) {
        rotation = 0f
        when (event.actionMasked) {
            MotionEvent.ACTION_DOWN -> {
                pointerId1 = event.getPointerId(event.actionIndex)
            }
            MotionEvent.ACTION_POINTER_DOWN -> {
                val pointerId1 = this.pointerId1
                pointerId1 ?: return
                val pointerId2 = event.getPointerId(event.actionIndex)
                this.pointerId2 = pointerId2

                val pointerIndex1 = event.findPointerIndex(pointerId1)
                val pointerIndex2 = event.findPointerIndex(pointerId2)
                if (pointerIndex1 < 0 || pointerIndex2 < 0) {
                    return
                }

                pointer1StartX = event.getX(pointerIndex1)
                pointer1StartY = event.getY(pointerIndex1)
                pointer2StartX = event.getX(pointerIndex2)
                pointer2StartY = event.getY(pointerIndex2)
            }
            MotionEvent.ACTION_MOVE -> {
                val pointerId1 = this.pointerId1 ?: return
                val pointerId2 = this.pointerId2 ?: return

                val pointerIndex1 = event.findPointerIndex(pointerId1)
                val pointerIndex2 = event.findPointerIndex(pointerId2)
                if (pointerIndex1 < 0 || pointerIndex2 < 0) {
                    return
                }

                val pointer1X = event.getX(pointerIndex1)
                val pointer1Y = event.getY(pointerIndex1)
                val pointer2X = event.getX(pointerIndex2)
                val pointer2Y = event.getY(pointerIndex2)
                rotation = angleBetweenLines(pointer1StartX, pointer1StartY, pointer2StartX, pointer2StartY,
                        pointer1X, pointer1Y, pointer2X, pointer2Y)
            }
            MotionEvent.ACTION_UP -> {
                pointerId1 = null
            }
            MotionEvent.ACTION_POINTER_UP -> {
                pointerId2 = null
            }
            MotionEvent.ACTION_CANCEL -> {
                pointerId1 = null
                pointerId2 = null
            }
        }
    }

    private fun angleBetweenLines(line1Point1X: Float, line1Point1Y: Float, line1Point2X: Float, line1Point2Y: Float,
                                  line2Point1X: Float, line2Point1Y: Float, line2Point2X: Float, line2Point2Y: Float
    ): Float {
        val angle1 = Math.atan2((line1Point1Y - line1Point2Y).toDouble(), (line1Point1X - line1Point2X).toDouble()).toFloat()
        val angle2 = Math.atan2((line2Point1Y - line2Point2Y).toDouble(), (line2Point1X - line2Point2X).toDouble()).toFloat()
        return angle2 - angle1
    }
}
