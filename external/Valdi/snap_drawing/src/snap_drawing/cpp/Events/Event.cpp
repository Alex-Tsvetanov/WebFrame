//
//  Event.cpp
//  snap_drawing-pc
//
//  Created by Simon Corsin on 1/19/22.
//

#include "snap_drawing/cpp/Events/Event.hpp"

namespace snap::drawing {

Event::Event(EventId id, snap::drawing::TimePoint time, EventCallback&& callback) noexcept
    : id(id), time(time), callback(std::move(callback)) {}

Event::Event(Event&& other) noexcept : id(other.id), time(other.time), callback(std::move(other.callback)) {
    other.id = EventId();
}

Event& Event::operator=(Event&& other) noexcept {
    id = other.id;
    time = other.time;
    callback = std::move(other.callback);

    other.id = EventId();

    return *this;
}

bool Event::cancel() {
    if (!callback) {
        return false;
    }
    callback = EventCallback();
    return true;
}

} // namespace snap::drawing
