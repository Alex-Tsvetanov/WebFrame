//
//  SkiaGestureRecognizer.cpp
//  valdi-skia
//
//  Created by Simon Corsin on 6/29/20.
//

#include "snap_drawing/cpp/Touches/GestureRecognizer.hpp"
#include "snap_drawing/cpp/Layers/Layer.hpp"
#include <fmt/format.h>

namespace snap::drawing {

GestureRecognizer::GestureRecognizer() = default;

GestureRecognizer::~GestureRecognizer() = default;

GestureRecognizerState GestureRecognizer::getState() const {
    return _state;
}

bool GestureRecognizer::shouldBegin() {
    return true;
}

bool GestureRecognizer::requiresFailureOf(const GestureRecognizer& /*other*/) const {
    return false;
}

bool GestureRecognizer::canRecognizeSimultaneously(const GestureRecognizer& /*other*/) const {
    return false;
}

bool GestureRecognizer::isActive() const {
    return _state == GestureRecognizerStateBegan || _state == GestureRecognizerStateChanged ||
           _state == GestureRecognizerStateEnded;
}

const TouchEvent* GestureRecognizer::getLastEvent() const {
    if (!_lastEvent.has_value()) {
        return nullptr;
    }
    return &_lastEvent.value();
}

Point GestureRecognizer::getLocation() const {
    if (!_lastEvent.has_value()) {
        return Point::make(0, 0);
    }
    return _lastEvent.value().getLocation();
}

Point GestureRecognizer::getLocationInWindow() const {
    if (!_lastEvent.has_value()) {
        return Point::make(0, 0);
    }
    return _lastEvent.value().getLocationInWindow();
}

void GestureRecognizer::update(const TouchEvent& event) {
    auto wasPossible = _state == GestureRecognizerStatePossible;
    if (_state == GestureRecognizerStateBegan) {
        _state = GestureRecognizerStateChanged;
    }

    onUpdate(event);

    if (isActive()) {
        _lastEvent = event;

        if (wasPossible && !shouldBegin()) {
            _state = GestureRecognizerStateFailed;
        }
    }
}

void GestureRecognizer::transitionToState(GestureRecognizerState newState) {
    _state = newState;
}

void GestureRecognizer::process() {
    _wasProcessed = true;
    onProcess();
}

void GestureRecognizer::cancel() {
    if (_wasProcessed && _state != GestureRecognizerStateEnded) {
        // We send an ended event if we were already started (thus acting like a cancel)
        _state = GestureRecognizerStateEnded;
        process();
    }

    _wasProcessed = false;
    _state = GestureRecognizerStatePossible;
    _lastEvent = std::nullopt;
    onReset();
}

void GestureRecognizer::onStarted() {}

void GestureRecognizer::onReset() {}

void GestureRecognizer::setLayer(ILayer* layer) {
    if (layer != nullptr) {
        _layer = Valdi::weakRef(layer);
    } else {
        _layer.reset();
    }
}

Valdi::Ref<ILayer> GestureRecognizer::getLayer() const {
    return _layer.lock();
}

void GestureRecognizer::setShouldCancelOtherGesturesOnStart(bool shouldCancelOtherGesturesOnStart) {
    _shouldCancelOtherGesturesOnStart = shouldCancelOtherGesturesOnStart;
}

bool GestureRecognizer::shouldCancelOtherGesturesOnStart() const {
    return _shouldCancelOtherGesturesOnStart;
}

bool GestureRecognizer::shouldProcessBeforeOtherGestures() const {
    return _shouldProcessBeforeOtherGestures;
}

void GestureRecognizer::setShouldProcessBeforeOtherGestures(bool shouldProcessBeforeOtherGestures) {
    _shouldProcessBeforeOtherGestures = shouldProcessBeforeOtherGestures;
}

static std::string_view gestureRecognizerStateToString(GestureRecognizerState state) {
    switch (state) {
        case GestureRecognizerStatePossible:
            return "possible";
        case GestureRecognizerStateFailed:
            return "failed";
        case GestureRecognizerStateBegan:
            return "began";
        case GestureRecognizerStateChanged:
            return "changed";
        case GestureRecognizerStateEnded:
            return "ended";
    }
}

std::string GestureRecognizer::toDebugString() const {
    auto* layer = dynamic_cast<Layer*>(getLayer().get());
    const char* accessibilityId = "";
    if (layer != nullptr) {
        accessibilityId = layer->getAccessibilityId().getCStr();
    }

    return fmt::format("Gesture '{}' with ID '{}' and state {}",
                       getTypeName(),
                       accessibilityId,
                       gestureRecognizerStateToString(_state));
}

} // namespace snap::drawing
