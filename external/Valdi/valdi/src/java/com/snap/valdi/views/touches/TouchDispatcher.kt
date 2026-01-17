package com.snap.valdi.views.touches

import android.graphics.Rect
import android.os.Build
import android.view.MotionEvent
import android.view.View
import android.view.ViewGroup
import com.snap.valdi.extensions.ViewUtils
import com.snap.valdi.logger.Logger
import com.snap.valdi.views.ValdiRootView

interface TouchDispatcher {

    val rootView: ViewGroup
    var captureAllHitTargets: Boolean

    var disallowInterceptTouchEventMode: DisallowInterceptTouchEventMode

    var cancelsTouchTargetsWhenGestureRequestsExclusivity: Boolean

    var adjustCoordinates: Boolean

    fun dispatchTouch(event: MotionEvent): Boolean

    fun getCurrentGesturesState(): GesturesState

    fun gestureRecognizerWantsDeferredUpdate(gestureRecognizer: ValdiGestureRecognizer): Boolean

    fun isEmpty(): Boolean

    companion object {
        fun hitTest(view: View, event: MotionEvent, outLocation: IntArray, rect: Rect): Boolean {
            view.getLocationOnScreen(outLocation)
            rect.left = outLocation[0]
            rect.top = outLocation[1]
            rect.right = outLocation[0] + view.width
            rect.bottom = outLocation[1] + view.height
            return if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q && newMultiTouchExperience(view)) {
                rect.contains(event.getRawX(event.actionIndex).toInt(), event.getRawY(event.actionIndex).toInt())
            } else {
                rect.contains(event.rawX.toInt(), event.rawY.toInt())
            }
        }

        fun create(
            rootView: ViewGroup,
            disallowInterceptTouchEventMode: DisallowInterceptTouchEventMode,
            logger: Logger?,
            debugTouchEvents: Boolean = false,
            cancelsTouchTargetsWhenGestureRequestsExclusivity: Boolean = false,
            captureAllHitTargets: Boolean = false,
            adjustCoordinates: Boolean = false,
        ) : TouchDispatcher {
            return if (newMultiTouchExperience(rootView)) {
                TouchDispatcherNewExperience(
                    rootView,
                    disallowInterceptTouchEventMode,
                    logger,
                    debugTouchEvents,
                    cancelsTouchTargetsWhenGestureRequestsExclusivity,
                    captureAllHitTargets,
                    adjustCoordinates,
                )
            } else {
                TouchDispatcherImpl(
                    rootView,
                    disallowInterceptTouchEventMode,
                    logger,
                    debugTouchEvents,
                    cancelsTouchTargetsWhenGestureRequestsExclusivity,
                    captureAllHitTargets,
                    adjustCoordinates,
                )
            }
        }

        fun newMultiTouchExperience(view: View): Boolean {
            val rootView = if (view is ValdiRootView) {
                view
            } else {
                ViewUtils.findValdiContext(view)?.rootView
            }
            return rootView?.useNewMultiTouchExperience ?: false
        }
    }
}