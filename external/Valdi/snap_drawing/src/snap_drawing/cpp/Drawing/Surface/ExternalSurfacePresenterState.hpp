//
//  ExternalSurfacePresenterState.hpp
//  snap_drawing-macos
//
//  Created by Simon Corsin on 2/7/22.
//

#pragma once

#include "snap_drawing/cpp/Utils/Aliases.hpp"
#include "snap_drawing/cpp/Utils/Geometry.hpp"
#include "snap_drawing/cpp/Utils/Matrix.hpp"
#include "snap_drawing/cpp/Utils/Path.hpp"

namespace snap::drawing {

struct ExternalSurfacePresenterState {
    Rect frame;
    Matrix transform;
    Path clipPath;
    Scalar opacity = 0;

    ExternalSurfacePresenterState();
    ExternalSurfacePresenterState(const Rect& frame, const Matrix& transform, const Path& clipPath, Scalar opacity);

    bool operator==(const ExternalSurfacePresenterState& other) const;
    bool operator!=(const ExternalSurfacePresenterState& other) const;
};

} // namespace snap::drawing
