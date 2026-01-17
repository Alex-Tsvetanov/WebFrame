//
//  ValueInterpolators.hpp
//  valdi-desktop-apple
//
//  Created by Simon Corsin on 7/3/20.
//

#pragma once

#include "snap_drawing/cpp/Utils/BorderRadius.hpp"
#include "snap_drawing/cpp/Utils/Color.hpp"
#include "snap_drawing/cpp/Utils/Geometry.hpp"

namespace snap::drawing {

Rect interpolateValue(const Rect& from, const Rect& to, double ratio);
Size interpolateValue(const Size& from, const Size& to, double ratio);
Point interpolateValue(const Point& from, const Point& to, double ratio);
Scalar interpolateValue(Scalar from, Scalar to, double ratio);
Color interpolateValue(Color from, Color to, double ratio);

BorderRadius interpolateBorderRadius(BorderRadius from, BorderRadius to, const Rect& bounds, double ratio);

} // namespace snap::drawing
