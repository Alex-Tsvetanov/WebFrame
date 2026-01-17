//
//  EventId.hpp
//  snap_drawing-pc
//
//  Created by Simon Corsin on 1/19/22.
//

#pragma once

#include <cstdint>

namespace snap::drawing {

struct EventId {
    uint32_t index = 0;
    uint32_t sequence = 0;

    constexpr EventId() = default;
    constexpr EventId(uint32_t index, uint32_t sequence) : index(index), sequence(sequence) {}

    constexpr bool operator==(const EventId& other) const {
        return index == other.index && sequence == other.sequence;
    }

    constexpr bool operator!=(const EventId& other) const {
        return !(*this == other);
    }
};

} // namespace snap::drawing
