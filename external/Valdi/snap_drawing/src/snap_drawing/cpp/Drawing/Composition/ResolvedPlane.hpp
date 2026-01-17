//
//  ResolvedPlane.hpp
//  snap_drawing-macos
//
//  Created by Simon Corsin on 2/7/22.
//

#pragma once

#include "snap_drawing/cpp/Drawing/Surface/ExternalSurface.hpp"
#include "snap_drawing/cpp/Drawing/Surface/ExternalSurfacePresenterState.hpp"
#include "snap_drawing/cpp/Utils/BoundingBoxHierarchy.hpp"

#include <variant>

namespace snap::drawing {

struct ResolvedRegularPlane {
    // Index of the surface inside the DisplayList
    uint64_t planeIndex;
    // Bounding box calculator, set if the surface is not external
    Ref<BoundingBoxHierarchy> bbox;

    inline ResolvedRegularPlane(uint64_t planeIndex, const Ref<BoundingBoxHierarchy>& bbox)
        : planeIndex(planeIndex), bbox(bbox) {}
};

struct ResolvedExternalPlane {
    ExternalSurfaceSnapshot* externalSurface;
    Matrix transform;
    Path clipPath;
    Scalar opacity;
    Rect absoluteFrame;

    inline ResolvedExternalPlane(ExternalSurfaceSnapshot* externalSurface,
                                 const Matrix& transform,
                                 const Path& clipPath,
                                 Scalar opacity,
                                 const Rect& absoluteFrame)
        : externalSurface(externalSurface),
          transform(transform),
          clipPath(clipPath),
          opacity(opacity),
          absoluteFrame(absoluteFrame) {}

    ExternalSurfacePresenterState resolveDisplayState() const;
};

// Represents a surface which can be either external or regular.
// Regular surfaces are appended into the output display list,
// whereas
class ResolvedPlane {
public:
    ResolvedPlane(uint64_t planeIndex, const Ref<BoundingBoxHierarchy>& bbox);

    ResolvedPlane(ExternalSurfaceSnapshot* externalSurface,
                  const Matrix& transform,
                  const Path& clipPath,
                  Scalar opacity,
                  const Rect& absoluteFrame);

    ResolvedRegularPlane* getRegular();

    ResolvedExternalPlane* getExternal();

    const ResolvedRegularPlane* getRegular() const;

    const ResolvedExternalPlane* getExternal() const;

private:
    std::variant<ResolvedExternalPlane, ResolvedRegularPlane> _data;
};

} // namespace snap::drawing
