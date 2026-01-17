//
//  DragGestureRecognizer.cpp
//  valdi-skia
//
//  Created by Simon Corsin on 6/29/20.
//

#include "snap_drawing/cpp/Touches/DragGestureRecognizer.hpp"
#include "snap_drawing/cpp/Touches/PinchGestureRecognizer.hpp"
#include "snap_drawing/cpp/Touches/RotateGestureRecognizer.hpp"
#include "snap_drawing/cpp/Touches/TapGestureRecognizer.hpp"

#include "utils/debugging/Assert.hpp"
#include <cmath>

namespace snap::drawing {

DragGestureRecognizer::DragGestureRecognizer(const GesturesConfiguration& gesturesConfiguration)
    : _dragThreshold(gesturesConfiguration.dragTouchSlop) {}

DragGestureRecognizer::~DragGestureRecognizer() = default;

bool DragGestureRecognizer::requiresFailureOf(const GestureRecognizer& other) const {
    return dynamic_cast<const DragGestureRecognizer*>(&other) != nullptr;
}

bool DragGestureRecognizer::canRecognizeSimultaneously(const GestureRecognizer& other) const {
    if (dynamic_cast<const PinchGestureRecognizer*>(&other) != nullptr) {
        return true;
    }
    if (dynamic_cast<const RotateGestureRecognizer*>(&other) != nullptr) {
        return true;
    }
    return false;
}

DragEvent DragGestureRecognizer::makeMoveEvent() const {
    return MoveGestureRecognizer<DragEvent>::makeMoveEvent();
}

bool DragGestureRecognizer::shouldStartMove(const TouchEvent& event) const {
    auto distance = Point::distance(_moveState->startEvent.getLocationInWindow(), event.getLocationInWindow());
    auto movedEnough = (distance >= _dragThreshold);
    auto multipleTouch = event.getPointerCount() > 1;
    return (movedEnough || multipleTouch);
}

bool DragGestureRecognizer::shouldContinueMove(const TouchEvent& event) const {
    // we continue drags even if pointers change, to support multitouch and composing gestures
    return event.getPointerCount() > 0;
}

std::string_view DragGestureRecognizer::getTypeName() const {
    return "drag";
}

} // namespace snap::drawing
