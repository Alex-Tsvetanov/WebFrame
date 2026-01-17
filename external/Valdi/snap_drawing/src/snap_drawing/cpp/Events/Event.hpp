//
//  Event.hpp
//  snap_drawing-pc
//
//  Created by Simon Corsin on 1/19/22.
//

#pragma once

#include "snap_drawing/cpp/Events/EventCallback.hpp"
#include "snap_drawing/cpp/Events/EventId.hpp"
#include "snap_drawing/cpp/Utils/TimePoint.hpp"

namespace snap::drawing {

struct Event {
    EventId id;
    TimePoint time;
    EventCallback callback;

    Event(EventId id, TimePoint time, EventCallback&& callback) noexcept;
    Event(Event&& other) noexcept;

    Event& operator=(Event&& other) noexcept;

    bool cancel();
};

} // namespace snap::drawing
