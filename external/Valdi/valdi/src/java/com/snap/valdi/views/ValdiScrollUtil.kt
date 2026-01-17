package com.snap.valdi.views

import android.graphics.Rect
import android.view.MotionEvent
import android.view.View
import android.view.ViewGroup
import com.snap.valdi.views.touches.TouchDispatcher
import com.snap.valdi.views.touches.DragGestureRecognizer

class ValdiScrollUtil {
    private val outLocation = IntArray(2)
    private val rect = Rect()

    // Find the first possible drag gesture candidate
    fun possibleViewDragGesture(view: View, event: MotionEvent): DragGestureRecognizer? {
        if (!TouchDispatcher.hitTest(view, event, outLocation, rect)) {
            return null
        }

        if (view is ValdiView) {
            val gesture = view.getDragGestureRecognizer()
            if (gesture != null) {
                return gesture
            }
        }

        if (view !is ViewGroup) {
            return null;
        }
        
        for (i in 0 until view.childCount) {
            val dragGesture = possibleViewDragGesture(view.getChildAt(i), event)
            if (dragGesture != null) {
                return dragGesture
            }
        }
        return null;
    }

    fun allPossibleViewDragGestures(view: View, event: MotionEvent, gestures: MutableList<DragGestureRecognizer> = mutableListOf()): List<DragGestureRecognizer> {

        if (!TouchDispatcher.hitTest(view, event, outLocation, rect)) {
            return gestures;
        }

        if (view is ValdiView) {
            val gesture = view.getDragGestureRecognizer()
            if (gesture != null) {
                gestures.add(gesture)
            }
        }

        if (view !is ViewGroup) {
            return gestures;
        }

        for (i in 0 until view.childCount) {
            allPossibleViewDragGestures(view.getChildAt(i), event, gestures)
        }

        return gestures;
    }

    fun canViewScroll(view: View, event: MotionEvent, check: (view: View) -> Boolean): Boolean {
        if (!TouchDispatcher.hitTest(view, event, outLocation, rect)) {
            return false
        }

        if (check(view)) {
            return true
        }

        if (view is ValdiView && view.hasDragGestureRecognizer()) {
            return true
        }

        if (view !is ViewGroup) {
            return false
        }

        for (i in 0 until view.childCount) {
            if (canViewScroll(view.getChildAt(i), event, check)) {
                return true
            }
        }

        return false
    }
}