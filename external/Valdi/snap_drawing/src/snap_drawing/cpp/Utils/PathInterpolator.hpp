//
//  PathInterpolator.hpp
//  snap_drawing
//
//  Created by Simon Corsin on 4/20/22.
//

#pragma once

#include "include/core/SkContourMeasure.h"
#include "snap_drawing/cpp/Utils/Path.hpp"
#include "valdi_core/cpp/Utils/SmallVector.hpp"

namespace snap::drawing {

class PathInterpolator {
public:
    explicit PathInterpolator(const Path& path);
    ~PathInterpolator();

    const Path& interpolate(Scalar start, Scalar end);

private:
    Path _interpolatedPath;
    Valdi::SmallVector<sk_sp<SkContourMeasure>, 1> _contours;
    Scalar _totalLength = 0.0f;
};

} // namespace snap::drawing
