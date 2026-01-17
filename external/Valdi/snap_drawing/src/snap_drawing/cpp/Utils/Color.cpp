//
//  Color.cpp
//  snap_drawing
//
//  Created by Simon Corsin on 1/28/22.
//

#include "snap_drawing/cpp/Utils/Color.hpp"
#include <fmt/format.h>

namespace snap::drawing {

std::string Color::toString() const {
    return fmt::format("{:#010x}", value);
}

std::ostream& operator<<(std::ostream& os, const Color& color) {
    return os << color.toString();
}

} // namespace snap::drawing
