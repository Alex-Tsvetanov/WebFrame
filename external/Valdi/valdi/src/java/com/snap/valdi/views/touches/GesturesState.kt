package com.snap.valdi.views.touches

/**
 * GesturesState provides some information about the state of the gestures system within
 * a ValdiRootView.
 */
enum class GesturesState {
    /**
     * No gestures are currently active. This will occur if the last dispatchTouchEvent() call
     * did not hit any elements with gestures attached, or if a stream of events ended up
     * causing all the captured gestures to fail. dispatchTouchEvent() will return false in this
     * scenario.
     */
    INACTIVE,

    /**
     * The gestures system has some gestures candidate that might trigger after more events come in,
     * but none of them have yet recognized a gesture. This will occur for instance if the last
     * dispatchTouchEvent() call hit elements with gestures attached. dispatchTouchEvent() will
     * return true in this scenario.
     */
    HAS_POSSIBLE_GESTURES,

    /**
     * Gestures have been recognized and are currently active. Their callbacks (like onDrag, onTap,
     * onLongPress etc...) have been fired. This will occur if a stream of events ended up causing
     * at least one capture gesture to be recognized. dispatchTouchEvent() will return true in
     * this scenario.
     */
    HAS_ACTIVE_GESTURES,
}