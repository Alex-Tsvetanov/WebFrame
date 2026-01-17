//
//  SkiaTouchEvent.hpp
//  valdi-skia-apple
//
//  Created by Simon Corsin on 6/27/20.
//

#pragma once

#include "valdi_core/cpp/Utils/SmallVector.hpp"

#include "snap_drawing/cpp/Utils/Aliases.hpp"
#include "snap_drawing/cpp/Utils/Geometry.hpp"
#include "snap_drawing/cpp/Utils/TimePoint.hpp"
#include <ostream>

namespace snap::drawing {

enum TouchEventType {
    TouchEventTypeDown,        // the user touched down
    TouchEventTypeMoved,       // the touch moved
    TouchEventTypeIdle,        // the touch is active but not moving
    TouchEventTypeUp,          // the user touched up
    TouchEventTypeWheel,       // the user moved the scrollwheel (or panned with a touchpad)
    TouchEventTypeNone,        // there are no longer any active touches
    TouchEventTypePointerUp,   // multitouch, when the user removes a pointer (lifts a finger up)
    TouchEventTypePointerDown, // multitouch, when the user adds a pointer (places a finger down)
};

class TouchEvent {
public:
    using PointerLocations = Valdi::SmallVector<Point, 2>;

    TouchEvent(TouchEventType type,
               const Point& locationFromWindow,
               const Point& location,
               const Vector& direction,
               size_t pointerCount,
               size_t actionIndex,
               const PointerLocations pointerLocations,
               const TimePoint& time,
               Duration offsetSinceSource,
               const Ref<Valdi::RefCountable>& source);
    ~TouchEvent();

    TouchEventType getType() const;

    const Point& getLocationInWindow() const;

    const Point& getLocation() const;

    const Vector& getDirection() const;

    Duration getOffsetSinceSource() const;

    size_t getPointerCount() const;

    size_t getActionIndex() const;

    const Point& getLocationByPointer(size_t pointer) const;

    const PointerLocations& getPointerLocations() const;

    const TimePoint& getTime() const;

    const Ref<Valdi::RefCountable>& getSource() const;

    std::string toString() const;

    TouchEvent withLocation(const Point& newLocation) const;

private:
    TouchEventType _type;
    Point _locationFromWindow;
    Point _location;
    Vector _direction;
    size_t _pointerCount;
    size_t _actionIndex;
    TouchEvent::PointerLocations _pointerLocations;
    TimePoint _time;
    Duration _offsetSinceSource;
    Ref<Valdi::RefCountable> _source;
};

std::ostream& operator<<(std::ostream& os, const TouchEvent& touchEvent) noexcept;

} // namespace snap::drawing
