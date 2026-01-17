package com.snap.valdi.views.touches

/**
 * Defines how the Valdi touch system will prevent touches from being intercepted by their parent.
 */
enum class DisallowInterceptTouchEventMode {

    /**
     * This mode causes the root view to start disallowing the view parents to intercept
     * the touch events as soon as a Valdi gesture is recognized. When touch events are processed,
     * view parents outside of Valdi are allowed to intercept the touch until the Valdi
     * gesture system recognizes a complete gesture, like a drag/scroll/tap/long press.
     *
     * This is the default mode.
     */
    DISALLOW_WHEN_GESTURE_RECOGNIZED,

    /**
     * This mode causes the root view to start disallowing the view parents to intercept the touch
     * events as soon as the root view receives events and that it has active gesture candidates.
     * Valdi gets exclusivity of the touch events, until the Valdi gesture system has either
     * decided that none of the gestures are recognized, or until all the gestures have finished
     * processing.
     *
     * This mode works best when Valdi is used in view parents that can deal with
     * requestDisallowInterceptTouchEvent() toggling dynamically during touch events, like
     * RecyclerView.
     */
    DISALLOW_WHEN_GESTURE_POSSIBLE,
}