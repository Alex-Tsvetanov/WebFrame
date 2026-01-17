//
//  SkiaTouchEvent.cpp
//  valdi-skia-apple
//
//  Created by Simon Corsin on 6/27/20.
//

#include "snap_drawing/cpp/Touches/TouchEvent.hpp"
#include "utils/debugging/Assert.hpp"
#include <fmt/format.h>
#include <fmt/ostream.h>

namespace snap::drawing {

TouchEvent::TouchEvent(TouchEventType type,
                       const Point& locationFromWindow,
                       const Point& location,
                       const Vector& direction,
                       size_t pointerCount,
                       size_t actionIndex,
                       TouchEvent::PointerLocations pointerLocations,
                       const TimePoint& timePoint,
                       Duration offsetSinceSource,
                       const Ref<Valdi::RefCountable>& source)
    : _type(type),
      _locationFromWindow(locationFromWindow),
      _location(location),
      _direction(direction),
      _pointerCount(pointerCount),
      _actionIndex(actionIndex),
      _pointerLocations(std::move(pointerLocations)),
      _time(timePoint),
      _offsetSinceSource(offsetSinceSource),
      _source(source) {}

TouchEvent::~TouchEvent() = default;

TouchEventType TouchEvent::getType() const {
    return _type;
}

const Point& TouchEvent::getLocationInWindow() const {
    return _locationFromWindow;
}

const Point& TouchEvent::getLocation() const {
    return _location;
}

const Vector& TouchEvent::getDirection() const {
    return _direction;
}

size_t TouchEvent::getPointerCount() const {
    return _pointerCount;
}

size_t TouchEvent::getActionIndex() const {
    return _actionIndex;
}

const Point& TouchEvent::getLocationByPointer(size_t pointer) const {
    SC_ASSERT(pointer < _pointerLocations.size());
    return _pointerLocations[pointer];
}

const TouchEvent::PointerLocations& TouchEvent::getPointerLocations() const {
    return _pointerLocations;
}

const TimePoint& TouchEvent::getTime() const {
    return _time;
}

const Ref<Valdi::RefCountable>& TouchEvent::getSource() const {
    return _source;
}

Duration TouchEvent::getOffsetSinceSource() const {
    return _offsetSinceSource;
}

TouchEvent TouchEvent::withLocation(const Point& newLocation) const {
    return TouchEvent(_type,
                      _locationFromWindow,
                      newLocation,
                      _direction,
                      _pointerCount,
                      _actionIndex,
                      _pointerLocations,
                      _time,
                      _offsetSinceSource,
                      _source);
}

std::string TouchEvent::toString() const {
    std::string_view type;
    switch (_type) {
        case TouchEventTypeDown:
            type = "down";
            break;
        case TouchEventTypeMoved:
            type = "moved";
            break;
        case TouchEventTypeIdle:
            type = "idle";
            break;
        case TouchEventTypeUp:
            type = "up";
            break;
        case TouchEventTypeWheel:
            type = "wheel";
            break;
        case TouchEventTypeNone:
            type = "none";
            break;
        case TouchEventTypePointerDown:
            type = "pointerdown";
            break;
        case TouchEventTypePointerUp:
            type = "pointerup";
            break;
    }

    return fmt::format(
        "touch type {} at window location {}, local location {}, pointerCount {}, actionIndex {}, time {}",
        type,
        _locationFromWindow,
        _location,
        _pointerCount,
        _actionIndex,
        _time.getTime());
}

std::ostream& operator<<(std::ostream& os, const TouchEvent& touchEvent) noexcept {
    return os << touchEvent.toString();
}

} // namespace snap::drawing
