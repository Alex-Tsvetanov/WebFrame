//
//  PinchGestureRecognizer.cpp
//  valdi-skia
//
//  Created by Simon Corsin on 6/29/20.
//
#include "snap_drawing/cpp/Touches/PinchGestureRecognizer.hpp"
#include "snap_drawing/cpp/Touches/DragGestureRecognizer.hpp"
#include "snap_drawing/cpp/Touches/RotateGestureRecognizer.hpp"

#include "utils/debugging/Assert.hpp"
#include <cmath>

namespace snap::drawing {

PinchGestureRecognizer::PinchGestureRecognizer(const GesturesConfiguration& /*gesturesConfiguration*/) {}

PinchGestureRecognizer::~PinchGestureRecognizer() = default;

bool PinchGestureRecognizer::requiresFailureOf(const GestureRecognizer& other) const {
    return dynamic_cast<const PinchGestureRecognizer*>(&other) != nullptr;
}

bool PinchGestureRecognizer::canRecognizeSimultaneously(const GestureRecognizer& other) const {
    if (dynamic_cast<const RotateGestureRecognizer*>(&other) != nullptr) {
        return true;
    }
    if (dynamic_cast<const DragGestureRecognizer*>(&other) != nullptr) {
        return true;
    }
    return false;
}

PinchEvent PinchGestureRecognizer::makeMoveEvent() const {
    PinchEvent pinchEvent = MoveGestureRecognizer<PinchEvent>::makeMoveEvent();
    if (pinchEvent.pointerCount > 1 && _moveState.value().currentEvent.getType() != TouchEventTypePointerUp &&
        _moveState.value().currentEvent.getType() != TouchEventTypePointerDown) {
        // for events with more than one pointer, we use the delta from the cached value
        pinchEvent.scale = getCurrentScale() * _netScale;
    } else {
        // currently, our gesture detection system fires one last event for the finger leaving with a null currentEvent,
        // and we want to ensure any changes are persisted
        pinchEvent.scale = _netScale;
    }

    return pinchEvent;
}

// (mli6) WARN) - startEvent setting for multitouch does not track perfectly accurately for resumptions -
// Ticket: 2885
Scalar PinchGestureRecognizer::getCurrentScale() const {
    auto startDirection = _moveState.value().startEvent.getDirection();
    auto currentDirection = _moveState.value().currentEvent.getDirection();

    auto startScale = Point::length(startDirection.dx, startDirection.dy);
    auto currentScale = Point::length(currentDirection.dx, currentDirection.dy);

    return currentScale / startScale;
}

bool PinchGestureRecognizer::shouldStartMove(const TouchEvent& event) const {
    return event.getPointerCount() > 1;
}

bool PinchGestureRecognizer::shouldContinueMove(const TouchEvent& event) const {
    return event.getPointerCount() > 0;
}

void PinchGestureRecognizer::onPointerChange(const TouchEvent& event) {
    if (event.getPointerCount() == 2 && event.getType() == TouchEventTypePointerUp) {
        // if any touches are still present, we allow for a scale to continue
        // when end users lift all but one finger, we cache the current scale amount such that they can resume the
        // scale

        // we only cache this when the pointer count decreases, such that multifinger events or duplicate events don't
        // cause a re-caching
        _netScale *= getCurrentScale();
        transitionToState(GestureRecognizerStateChanged);
    }
}

void PinchGestureRecognizer::onReset() {
    MoveGestureRecognizer::onReset();
    _netScale = 1;
}

std::string_view PinchGestureRecognizer::getTypeName() const {
    return "pinch";
}
} // namespace snap::drawing
