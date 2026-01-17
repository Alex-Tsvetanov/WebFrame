//
//  EventCallback.hpp
//  snap_drawing-pc
//
//  Created by Simon Corsin on 1/19/22.
//

#pragma once

#include "snap_drawing/cpp/Utils/TimePoint.hpp"
#include "valdi_core/cpp/Utils/Function.hpp"

namespace snap::drawing {

using EventCallback = Valdi::Function<void(TimePoint, Duration)>;

}
