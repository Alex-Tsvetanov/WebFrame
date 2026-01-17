//
//  GesturesConfiguration.cpp
//  snap_drawing-pc
//
//  Created by Simon Corsin on 11/17/22.
//

#include "snap_drawing/cpp/Touches/GesturesConfiguration.hpp"

namespace snap::drawing {

constexpr auto kDefaultLongPressTimeout = Duration(0.25);
constexpr auto kDefaultDoubleTapTimeout = Duration(0.25);
constexpr Scalar kDefaultDragTouchSlop = 10.0f;
constexpr Scalar kDefaultTouchTolerance = 5.0f;
constexpr Scalar kDefaultScrollFriction = 0.015f;

GesturesConfiguration GesturesConfiguration::getDefault() {
    return GesturesConfiguration(kDefaultLongPressTimeout,
                                 kDefaultDoubleTapTimeout,
                                 kDefaultDragTouchSlop,
                                 kDefaultTouchTolerance,
                                 kDefaultScrollFriction,
                                 /* debugGestures */ false);
}

} // namespace snap::drawing
