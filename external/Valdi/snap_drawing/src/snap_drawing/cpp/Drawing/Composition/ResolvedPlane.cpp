//
//  ResolvedPlane.cpp
//  snap_drawing-macos
//
//  Created by Simon Corsin on 2/7/22.
//

#include "snap_drawing/cpp/Drawing/Composition/ResolvedPlane.hpp"

namespace snap::drawing {

ExternalSurfacePresenterState ResolvedExternalPlane::resolveDisplayState() const {
    auto relativeSize = externalSurface->getExternalSurface()->getRelativeSize();

    Matrix resolvedTransform;
    Rect displayFrame;
    if (transform.isIdentityOrTranslate()) {
        // If we have just a translation transform, we put the translation in the display frame
        // and pass in an identity transform. This makes the rendering more compatible with platforms
        // that expect a proper frame position on their view elements.
        displayFrame = Rect::makeXYWH(
            transform.getTranslateX(), transform.getTranslateY(), relativeSize.width, relativeSize.height);
    } else {
        // If we have a more complex transform, the view will have to be positioned using the transform
        // starting at 0,0 .
        resolvedTransform = transform;
        displayFrame = Rect::makeXYWH(0, 0, relativeSize.width, relativeSize.height);
    }

    return ExternalSurfacePresenterState(displayFrame, resolvedTransform, clipPath, opacity);
}

ResolvedPlane::ResolvedPlane(uint64_t planeIndex, const Ref<BoundingBoxHierarchy>& bbox)
    : _data(ResolvedRegularPlane(planeIndex, bbox)) {}

ResolvedPlane::ResolvedPlane(ExternalSurfaceSnapshot* externalSurface,
                             const Matrix& transform,
                             const Path& clipPath,
                             Scalar opacity,
                             const Rect& absoluteFrame)
    : _data(ResolvedExternalPlane(externalSurface, transform, clipPath, opacity, absoluteFrame)) {}

ResolvedRegularPlane* ResolvedPlane::getRegular() {
    return std::get_if<ResolvedRegularPlane>(&_data);
}

ResolvedExternalPlane* ResolvedPlane::getExternal() {
    return std::get_if<ResolvedExternalPlane>(&_data);
}

const ResolvedRegularPlane* ResolvedPlane::getRegular() const {
    return std::get_if<ResolvedRegularPlane>(&_data);
}

const ResolvedExternalPlane* ResolvedPlane::getExternal() const {
    return std::get_if<ResolvedExternalPlane>(&_data);
}

} // namespace snap::drawing
