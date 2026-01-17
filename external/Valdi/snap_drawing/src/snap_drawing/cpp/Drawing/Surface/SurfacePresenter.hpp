//
//  SurfacePresenter.hpp
//  snap_drawing-macos
//
//  Created by Simon Corsin on 2/16/22.
//

#pragma once

#include "snap_drawing/cpp/Drawing/Surface/DrawableSurface.hpp"
#include "snap_drawing/cpp/Drawing/Surface/ExternalSurface.hpp"
#include "snap_drawing/cpp/Drawing/Surface/ExternalSurfacePresenterState.hpp"
#include "snap_drawing/cpp/Utils/TimePoint.hpp"

#include <optional>

namespace snap::drawing {

using SurfacePresenterId = size_t;

/**
 A SurfacePresenter is an object that can display a drawable surface
 or external surface. It has a id, which is used to identify it
 across the SurfaceManagerPresenter API. When the presenter is drawable,
 it should eventually have a DrawableSurface attached so that the DrawLooper
 can draw into it.
 */
class SurfacePresenter {
public:
    explicit SurfacePresenter(SurfacePresenterId id);
    ~SurfacePresenter();

    SurfacePresenterId getId() const;

    bool isExternal() const;
    bool isDrawable() const;

    ExternalSurface* getExternalSurface() const;
    DrawableSurface* getDrawableSurface() const;

    const std::optional<TimePoint>& getLastDrawnFrameTime() const;
    void setLastDrawnFrameTime(const std::optional<TimePoint>& lastDrawnFrameTime);

    bool needsDrawForFrameTime(const TimePoint& time) const;

    void setAsExternal(ExternalSurface& externalSurface, const ExternalSurfacePresenterState& presenterState);
    void setAsDrawable();

    void setSurface(Surface* surface);

    bool needsSynchronousDraw() const;
    void setNeedsSynchronousDraw(bool needsSynchronousDraw);

    void setDisplayListPlaneIndex(size_t displayListPlaneIndex);
    size_t getDisplayListPlaneIndex() const;

    const ExternalSurfacePresenterState* getExternalSurfacePresenterState() const;
    ExternalSurfacePresenterState* getExternalSurfacePresenterState();

private:
    SurfacePresenterId _id;
    Ref<Surface> _surface;
    size_t _displayListPlaneIndex = 0;
    std::optional<TimePoint> _lastDrawnFrameTime;
    std::optional<ExternalSurfacePresenterState> _externalSurfacePresenterState;
    bool _needsSynchronousDraw = false;
};

} // namespace snap::drawing
