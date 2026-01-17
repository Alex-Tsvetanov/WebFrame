//
//  ExternalSurfacePresenterState.cpp
//  snap_drawing-macos
//
//  Created by Simon Corsin on 2/7/22.
//

#include "snap_drawing/cpp/Drawing/Surface/ExternalSurfacePresenterState.hpp"

namespace snap::drawing {

ExternalSurfacePresenterState::ExternalSurfacePresenterState() = default;
ExternalSurfacePresenterState::ExternalSurfacePresenterState(const Rect& frame,
                                                             const Matrix& transform,
                                                             const Path& clipPath,
                                                             Scalar opacity)
    : frame(frame), transform(transform), clipPath(clipPath), opacity(opacity) {}

bool ExternalSurfacePresenterState::operator==(const ExternalSurfacePresenterState& other) const {
    return frame == other.frame && transform == other.transform && clipPath == other.clipPath &&
           opacity == other.opacity;
}

bool ExternalSurfacePresenterState::operator!=(const ExternalSurfacePresenterState& other) const {
    return !(*this == other);
}

} // namespace snap::drawing
