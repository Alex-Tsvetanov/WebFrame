//
//  SurfacePresenterManager.hpp
//  snap_drawing-macos
//
//  Created by Simon Corsin on 2/7/22.
//

#pragma once

#include "snap_drawing/cpp/Drawing/Surface/DrawableSurface.hpp"
#include "snap_drawing/cpp/Drawing/Surface/ExternalSurface.hpp"
#include "snap_drawing/cpp/Drawing/Surface/ExternalSurfacePresenterState.hpp"
#include "snap_drawing/cpp/Drawing/Surface/Surface.hpp"
#include "snap_drawing/cpp/Drawing/Surface/SurfacePresenter.hpp"
#include "snap_drawing/cpp/Utils/Aliases.hpp"

namespace snap::drawing {

/**
 The SurfacePresenterManager is responsible for displaying surfaces,
 which can be either DrawableSurface's or ExternalSurface's, while respecting their zIndex.
 */
class SurfacePresenterManager : public Valdi::SimpleRefCountable {
public:
    /**
     Create a presenter backed with a DrawableSurface for the given id, inserted at the given zIndex.
     If the backing DrawableSurface of the presenter can be created synchronously, it can be returned
     from this function. If returning null, the DrawableSurface for the given presenterId will have to
     be later set on the DrawLooper so that the looper can draw the content into the surface.
     */
    virtual Ref<DrawableSurface> createPresenterWithDrawableSurface(SurfacePresenterId presenterId, size_t zIndex) = 0;

    /**
     Create a presenter wrapping an ExternalSurface for the given id, inserted at the given zIndex.
     */
    virtual void createPresenterForExternalSurface(SurfacePresenterId presenterId,
                                                   size_t zIndex,
                                                   ExternalSurface& externalSurface) = 0;

    /**
     Update the zIndex for the presenter at the given id.
     */
    virtual void setSurfacePresenterZIndex(SurfacePresenterId presenterId, size_t zIndex) = 0;

    /**
     Update the presenter state for a presenter wrapping an ExternalSurface at the given id.
     */
    virtual void setExternalSurfacePresenterState(SurfacePresenterId presenterId,
                                                  const ExternalSurfacePresenterState& presenterState,
                                                  const ExternalSurfacePresenterState* previousPresenterState) = 0;

    /**
     Remove the presenter at the given id.
     */
    virtual void removeSurfacePresenter(SurfacePresenterId presenterId) = 0;

    /**
     Called whenever the DrawableSurface attached to the SurfacePresenter has been drawn into.
     */
    virtual void onDrawableSurfacePresenterUpdated(SurfacePresenterId presenterId);
};

} // namespace snap::drawing
