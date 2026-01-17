#include "snap_drawing/cpp/Touches/WheelGestureRecognizer.hpp"

#include <iostream>

namespace snap::drawing {

WheelGestureRecognizer::WheelGestureRecognizer(WheelGestureRecognizerListener&& listener)
    : _listener(std::move(listener)) {}

WheelGestureRecognizer::~WheelGestureRecognizer() = default;

bool WheelGestureRecognizer::canRecognizeSimultaneously(const GestureRecognizer& /*other*/) const {
    return true;
}

void WheelGestureRecognizer::onProcess() {
    if (!_listener) {
        return;
    }

    const auto* event = getLastEvent();
    WheelScrollEvent scrollEvent;
    scrollEvent.direction = event->getDirection();
    scrollEvent.location = event->getLocation();
    _listener(*this, getState(), scrollEvent);
    transitionToState(GestureRecognizerStateEnded);
}

void WheelGestureRecognizer::onUpdate(const TouchEvent& event) {
    switch (event.getType()) {
        case TouchEventTypeWheel:
            transitionToState(GestureRecognizerStateBegan);
            break;
        case TouchEventTypeDown:
        case TouchEventTypeIdle:
        case TouchEventTypeMoved:
        case TouchEventTypeUp:
        case TouchEventTypeNone:
        case TouchEventTypePointerUp:
        case TouchEventTypePointerDown: {
            transitionToState(GestureRecognizerStateFailed);
        } break;
    }
}

std::string_view WheelGestureRecognizer::getTypeName() const {
    return "drag";
}
} // namespace snap::drawing
