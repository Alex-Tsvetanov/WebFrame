//
//  FontMetrics.hpp
//  snap_drawing
//
//  Created by Simon Corsin on 11/14/22.
//

#pragma once

#include "snap_drawing/cpp/Utils/Scalar.hpp"

namespace snap::drawing {

struct FontMetrics {
    Scalar lineSpacing = 0.0f;
    Scalar ascent = 0.0f;
    Scalar descent = 0.0f;
    Scalar underlineThickness = 0.0f; //!< underline thickness
    Scalar underlinePosition = 0.0f;  //!< distance from baseline to top of stroke, typically positive
    Scalar strikeoutThickness = 0.0f; //!< strikeout thickness
    Scalar strikeoutPosition = 0.0f;  //!< distance from baseline to bottom of stroke, typically negative
};

} // namespace snap::drawing
