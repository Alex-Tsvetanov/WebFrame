//
//  EventQueue.hpp
//  valdi-desktop-apple
//
//  Created by Simon Corsin on 7/7/20.
//

#pragma once

#include "snap_drawing/cpp/Events/Event.hpp"

#include <deque>
#include <vector>

namespace snap::drawing {

class EventQueue {
public:
    explicit EventQueue(TimePoint initialTime);

    void flush(TimePoint currentTime);

    EventId enqueue(Duration delay, EventCallback&& callback);
    EventId enqueue(TimePoint time, EventCallback&& callback);
    bool cancel(EventId eventId);

    void clear();

    bool isEmpty() const;

private:
    std::vector<Event> _nextEvents;
    std::deque<Event> _pendingEvents;
    TimePoint _lastTime;
    uint32_t _sequence = 0;

    void collectNextEvents(TimePoint currentTime);

    bool cancelFromPendingEvents(EventId eventId);
    bool cancelFromProcessingEvents(EventId eventId);
};

} // namespace snap::drawing
