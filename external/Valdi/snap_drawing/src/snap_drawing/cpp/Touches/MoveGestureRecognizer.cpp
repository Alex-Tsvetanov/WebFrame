//
//  MoveGestureRecognizer.cpp
//  valdi-skia
//
//  Created by Vincent Brunet on 01/17/21.
//

#include "snap_drawing/cpp/Touches/MoveGestureRecognizer.hpp"
#include "snap_drawing/cpp/Touches/PinchGestureRecognizer.hpp"
#include "snap_drawing/cpp/Touches/RotateGestureRecognizer.hpp"

#include "utils/debugging/Assert.hpp"
#include <cmath>

namespace snap::drawing {

MoveGestureState::MoveGestureState(const TouchEvent& startEvent,
                                   const TouchEvent& lastEvent,
                                   const TouchEvent& currentEvent)
    : startEvent(startEvent), lastEvent(lastEvent), currentEvent(currentEvent) {}

} // namespace snap::drawing
