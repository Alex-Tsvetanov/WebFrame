//
//  SurfacePresenter.cpp
//  snap_drawing-macos
//
//  Created by Simon Corsin on 2/16/22.
//

#include "SurfacePresenter.hpp"

namespace snap::drawing {

SurfacePresenter::SurfacePresenter(SurfacePresenterId id) : _id(id) {}
SurfacePresenter::~SurfacePresenter() = default;

SurfacePresenterId SurfacePresenter::getId() const {
    return _id;
}

bool SurfacePresenter::isExternal() const {
    return _externalSurfacePresenterState.has_value();
}

bool SurfacePresenter::isDrawable() const {
    return !isExternal();
}

ExternalSurface* SurfacePresenter::getExternalSurface() const {
    return dynamic_cast<ExternalSurface*>(_surface.get());
}

DrawableSurface* SurfacePresenter::getDrawableSurface() const {
    return dynamic_cast<DrawableSurface*>(_surface.get());
}

void SurfacePresenter::setAsExternal(ExternalSurface& externalSurface,
                                     const ExternalSurfacePresenterState& presenterState) {
    _surface = &externalSurface;
    _externalSurfacePresenterState = {presenterState};
}

void SurfacePresenter::setAsDrawable() {
    _externalSurfacePresenterState = std::nullopt;
}

void SurfacePresenter::setSurface(Surface* surface) {
    _surface = surface;
}

void SurfacePresenter::setDisplayListPlaneIndex(size_t displayListPlaneIndex) {
    _displayListPlaneIndex = displayListPlaneIndex;
}

size_t SurfacePresenter::getDisplayListPlaneIndex() const {
    return _displayListPlaneIndex;
}

const ExternalSurfacePresenterState* SurfacePresenter::getExternalSurfacePresenterState() const {
    return _externalSurfacePresenterState.has_value() ? &_externalSurfacePresenterState.value() : nullptr;
}

ExternalSurfacePresenterState* SurfacePresenter::getExternalSurfacePresenterState() {
    return _externalSurfacePresenterState.has_value() ? &_externalSurfacePresenterState.value() : nullptr;
}

const std::optional<TimePoint>& SurfacePresenter::getLastDrawnFrameTime() const {
    return _lastDrawnFrameTime;
}

void SurfacePresenter::setLastDrawnFrameTime(const std::optional<TimePoint>& lastDrawnFrameTime) {
    _lastDrawnFrameTime = lastDrawnFrameTime;
}

bool SurfacePresenter::needsDrawForFrameTime(const TimePoint& time) const {
    return getDrawableSurface() != nullptr && (!_lastDrawnFrameTime || _lastDrawnFrameTime.value() != time);
}

bool SurfacePresenter::needsSynchronousDraw() const {
    return _needsSynchronousDraw;
}

void SurfacePresenter::setNeedsSynchronousDraw(bool needsSynchronousDraw) {
    _needsSynchronousDraw = needsSynchronousDraw;
}

} // namespace snap::drawing
