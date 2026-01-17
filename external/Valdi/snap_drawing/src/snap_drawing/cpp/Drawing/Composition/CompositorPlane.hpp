//
//  CompositorPlane.hpp
//  snap_drawing-macos
//
//  Created by Simon Corsin on 2/7/22.
//

#pragma once

#include "snap_drawing/cpp/Drawing/Surface/ExternalSurfacePresenterState.hpp"

namespace snap::drawing {

class ExternalSurfaceSnapshot;
class DrawableSurface;
class ExternalSurface;

enum CompositorPlaneType { CompositorPlaneTypeDrawable, CompositorPlaneTypeExternal };

class CompositorPlane {
public:
    CompositorPlane(const Ref<ExternalSurfaceSnapshot>& externalSurface,
                    const ExternalSurfacePresenterState& presenterState);
    explicit CompositorPlane(const Ref<DrawableSurface>& drawableSurface);
    CompositorPlane(CompositorPlane&& other);
    CompositorPlane(const CompositorPlane& other);
    ~CompositorPlane();

    CompositorPlaneType getType() const;

    DrawableSurface* getDrawableSurface() const;
    ExternalSurfaceSnapshot* getExternalSurfaceSnapshot() const;
    ExternalSurface* getExternalSurface() const;

    const ExternalSurfacePresenterState* getExternalSurfacePresenterState() const;
    ExternalSurfacePresenterState* getExternalSurfacePresenterState();

    CompositorPlane& operator=(CompositorPlane&& other);
    CompositorPlane& operator=(const CompositorPlane& other);

private:
    Ref<DrawableSurface> _drawableSurface;
    Ref<ExternalSurfaceSnapshot> _externalSurfaceSnapshot;
    ExternalSurfacePresenterState _externalSurfacePresenterState;
};

} // namespace snap::drawing
