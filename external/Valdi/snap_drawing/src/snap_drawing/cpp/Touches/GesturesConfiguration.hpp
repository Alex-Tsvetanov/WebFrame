//
//  GesturesConfiguration.hpp
//  snap_drawing-pc
//
//  Created by Simon Corsin on 11/17/22.
//

#pragma once

#include "snap_drawing/cpp/Utils/Duration.hpp"
#include "snap_drawing/cpp/Utils/Scalar.hpp"

namespace snap::drawing {

struct GesturesConfiguration {
    /**
     Defines the amount of time before a press is considered a long press.
     */
    Duration longPressTimeout;

    /**
     Defines the maximum amount of time for waiting for a second tap
     after a single tap.
     */
    Duration doubleTapTimeout;

    /**
     Defines the drag distance before a gesture is considered a drag.
     Any series of drag events above this value will be considered a drag.
     */
    Scalar dragTouchSlop;

    /**
     The tolerance for a touchable region to be considered touched.
     Currently only used for hit testing links inside labels, but might be used for hit testing
     layers in the future.
     */
    Scalar touchTolerance;

    /**
     The amount of friction when scrolling. This is a dimensionless value
     representing a coefficient applied when scrolling.
     Equivalent to Android's ViewConfiguration.getScrollFriction()
     (see https://developer.android.com/reference/android/view/ViewConfiguration#getScrollFriction() )
     */
    Scalar scrollFriction;

    /**
     Whether debug info should be printed when processing gestures
     */
    bool debugGestures;

    constexpr GesturesConfiguration(Duration longPressTimeout,
                                    Duration doubleTapTimeout,
                                    Scalar dragTouchSlop,
                                    Scalar touchTolerance,
                                    Scalar scrollFriction,
                                    bool debugGestures)
        : longPressTimeout(longPressTimeout),
          doubleTapTimeout(doubleTapTimeout),
          dragTouchSlop(dragTouchSlop),
          touchTolerance(touchTolerance),
          scrollFriction(scrollFriction),
          debugGestures(debugGestures) {}

    static GesturesConfiguration getDefault();
};

} // namespace snap::drawing
