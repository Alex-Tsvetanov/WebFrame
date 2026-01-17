//
//  Scalar.hpp
//  snap_drawing
//
//  Created by Simon Corsin on 1/28/22.
//

#pragma once

#include "include/core/SkScalar.h"

namespace snap::drawing {

using Scalar = SkScalar;

inline Scalar pixelsToScalar(int pixels, float pointScale) {
    return static_cast<Scalar>(pixels) / pointScale;
}

template<size_t length>
static inline bool scalarsEqual(const Scalar* left, const Scalar* right) {
    for (size_t i = 0; i < length; i++) {
        if (left[i] != right[i]) {
            return false;
        }
    }
    return true;
}

static inline Scalar sanitizeScalarFromScale(Scalar value, Scalar scale) {
    return roundf(value * scale) / scale;
}

} // namespace snap::drawing
