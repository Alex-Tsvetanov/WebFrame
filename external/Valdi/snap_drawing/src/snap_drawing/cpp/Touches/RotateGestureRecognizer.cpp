//
//  RotateGestureRecognizer.cpp
//  valdi-skia
//
//  Created by Simon Corsin on 6/29/20.
//
#include "snap_drawing/cpp/Touches/RotateGestureRecognizer.hpp"
#include "snap_drawing/cpp/Touches/DragGestureRecognizer.hpp"
#include "snap_drawing/cpp/Touches/PinchGestureRecognizer.hpp"

#include "utils/debugging/Assert.hpp"
#include <cmath>

namespace snap::drawing {

RotateGestureRecognizer::RotateGestureRecognizer(const GesturesConfiguration& /*gesturesConfiguration*/) {}

RotateGestureRecognizer::~RotateGestureRecognizer() = default;

bool RotateGestureRecognizer::requiresFailureOf(const GestureRecognizer& other) const {
    return dynamic_cast<const RotateGestureRecognizer*>(&other) != nullptr;
}

bool RotateGestureRecognizer::canRecognizeSimultaneously(const GestureRecognizer& other) const {
    if (dynamic_cast<const PinchGestureRecognizer*>(&other) != nullptr) {
        return true;
    }
    if (dynamic_cast<const DragGestureRecognizer*>(&other) != nullptr) {
        return true;
    }
    return false;
}

RotateEvent RotateGestureRecognizer::makeMoveEvent() const {
    RotateEvent rotateEvent = MoveGestureRecognizer<RotateEvent>::makeMoveEvent();

    if (rotateEvent.pointerCount > 1 && _moveState.value().currentEvent.getType() != TouchEventTypePointerUp &&
        _moveState.value().currentEvent.getType() != TouchEventTypePointerDown) {
        // for events with more than one pointer, we use the delta from the cached value
        rotateEvent.rotation = getCurrentRotation() + _netRotation;
    } else {
        // currently, our gesture detection system fires one last event for the finger leaving with a null currentEvent,
        // and we want to ensure any changes are persisted
        rotateEvent.rotation = _netRotation;
    }

    return rotateEvent;
}

// (mli6) WARN) - startEvent setting for multitouch does not track perfectly accurately for resumptions -
// Ticket: 2885
Scalar RotateGestureRecognizer::getCurrentRotation() const {
    auto startDirection = _moveState.value().startEvent.getDirection();
    auto currentDirection = _moveState.value().currentEvent.getDirection();
    auto startRotation = -atan2f(startDirection.dx, startDirection.dy);
    auto currentRotation = -atan2f(currentDirection.dx, currentDirection.dy);
    return currentRotation - startRotation;
}

bool RotateGestureRecognizer::shouldStartMove(const TouchEvent& event) const {
    return event.getPointerCount() > 1;
}

bool RotateGestureRecognizer::shouldContinueMove(const TouchEvent& event) const {
    return event.getPointerCount() > 0;
}

void RotateGestureRecognizer::onPointerChange(const TouchEvent& event) {
    if (event.getPointerCount() == 2 && event.getType() == TouchEventTypePointerUp) {
        // if any touches are still present, we allow for a rotation to continue
        // when end users lift all but one finger, we cache the current rotation amount such that they can resume the
        // rotation

        // we only cache this when the pointer count decreases, such that multifinger events or duplicate events don't
        // cause a re-caching
        _netRotation += getCurrentRotation();
        transitionToState(GestureRecognizerStateChanged);
    }
}

void RotateGestureRecognizer::onReset() {
    MoveGestureRecognizer::onReset();
    _netRotation = 0;
}

std::string_view RotateGestureRecognizer::getTypeName() const {
    return "rotate";
}

} // namespace snap::drawing
