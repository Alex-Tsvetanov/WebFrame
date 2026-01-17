//
//  CompositorPlane.cpp
//  snap_drawing-macos
//
//  Created by Simon Corsin on 2/7/22.
//

#include "snap_drawing/cpp/Drawing/Composition/CompositorPlane.hpp"
#include "snap_drawing/cpp/Drawing/Surface/DrawableSurface.hpp"
#include "snap_drawing/cpp/Drawing/Surface/ExternalSurface.hpp"

namespace snap::drawing {

CompositorPlane::CompositorPlane(const Ref<ExternalSurfaceSnapshot>& externalSurfaceSnapshot,
                                 const ExternalSurfacePresenterState& presenterState)
    : _externalSurfaceSnapshot(externalSurfaceSnapshot), _externalSurfacePresenterState(presenterState) {}
CompositorPlane::CompositorPlane(const Ref<DrawableSurface>& drawableSurface) : _drawableSurface(drawableSurface) {}

CompositorPlane::CompositorPlane(CompositorPlane&& other)
    : _drawableSurface(std::move(other._drawableSurface)),
      _externalSurfaceSnapshot(std::move(other._externalSurfaceSnapshot)),
      _externalSurfacePresenterState(std::move(other._externalSurfacePresenterState)) {}

CompositorPlane::CompositorPlane(const CompositorPlane& other)
    : _drawableSurface(other._drawableSurface),
      _externalSurfaceSnapshot(other._externalSurfaceSnapshot),
      _externalSurfacePresenterState(other._externalSurfacePresenterState) {}

CompositorPlane::~CompositorPlane() = default;

CompositorPlaneType CompositorPlane::getType() const {
    if (getExternalSurfaceSnapshot() == nullptr) {
        return CompositorPlaneTypeDrawable;
    } else {
        return CompositorPlaneTypeExternal;
    }
}

DrawableSurface* CompositorPlane::getDrawableSurface() const {
    return _drawableSurface.get();
}

ExternalSurfaceSnapshot* CompositorPlane::getExternalSurfaceSnapshot() const {
    return _externalSurfaceSnapshot.get();
}

ExternalSurface* CompositorPlane::getExternalSurface() const {
    if (_externalSurfaceSnapshot == nullptr) {
        return nullptr;
    }
    return _externalSurfaceSnapshot->getExternalSurface().get();
}

const ExternalSurfacePresenterState* CompositorPlane::getExternalSurfacePresenterState() const {
    if (getExternalSurfaceSnapshot() == nullptr) {
        return nullptr;
    }
    return &_externalSurfacePresenterState;
}

ExternalSurfacePresenterState* CompositorPlane::getExternalSurfacePresenterState() {
    if (getExternalSurfaceSnapshot() == nullptr) {
        return nullptr;
    }
    return &_externalSurfacePresenterState;
}

CompositorPlane& CompositorPlane::operator=(CompositorPlane&& other) {
    _externalSurfaceSnapshot = std::move(other._externalSurfaceSnapshot);
    _drawableSurface = std::move(other._drawableSurface);
    _externalSurfacePresenterState = std::move(other._externalSurfacePresenterState);
    return *this;
}

CompositorPlane& CompositorPlane::operator=(const CompositorPlane& other) {
    _externalSurfaceSnapshot = other._externalSurfaceSnapshot;
    _drawableSurface = other._drawableSurface;
    _externalSurfacePresenterState = other._externalSurfacePresenterState;
    return *this;
}

} // namespace snap::drawing
