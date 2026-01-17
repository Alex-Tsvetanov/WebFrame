//
//  SCValdiMacOSSurfacePresenterManager.hpp
//  valdi-desktop-apple
//
//  Created by Simon Corsin on 2/22/22.
//

#pragma once

#import "Foundation/Foundation.h"

#include "snap_drawing/apple/CoreGraphicsUtils.hpp"
#include "snap_drawing/cpp/Drawing/GraphicsContext/MetalGraphicsContext.hpp"
#include "snap_drawing/cpp/Drawing/Surface/SurfacePresenterManager.hpp"
#include "valdi_core/cpp/Utils/FlatMap.hpp"

@class CAMetalLayer;

@protocol SCSnapDrawingMetalSurfacePresenterManagerDelegate <NSObject>

- (id)createMetalPresenter:(inout CAMetalLayer**)metalLayer API_AVAILABLE(ios(13.0));

- (id)createEmbeddedPresenterForView:(id)view;

- (void)removePresenter:(id)presenter;

- (void)setZIndex:(NSUInteger)zIndex forPresenter:(id)presenter;

- (void)setFrame:(CGRect)frame
               transform:(CATransform3D)transform
                 opacity:(CGFloat)opacity
                clipPath:(CGPathRef)clipPath
          clipHasChanged:(BOOL)clipHasChanged
    forEmbeddedPresenter:(id)presenter;

@end

namespace snap::drawing {

class ExternalSurface;

class MetalSurfacePresenterManager : public SurfacePresenterManager {
public:
    MetalSurfacePresenterManager(id<SCSnapDrawingMetalSurfacePresenterManagerDelegate> delegate,
                                 const Ref<MetalGraphicsContext>& graphicsContext);

    ~MetalSurfacePresenterManager() override;

    Ref<DrawableSurface> createPresenterWithDrawableSurface(SurfacePresenterId presenterId, size_t zIndex) override;

    void createPresenterForExternalSurface(SurfacePresenterId presenterId,
                                           size_t zIndex,
                                           ExternalSurface& externalSurface) override;

    void setSurfacePresenterZIndex(SurfacePresenterId presenterId, size_t zIndex) override;

    void setExternalSurfacePresenterState(SurfacePresenterId presenterId,
                                          const ExternalSurfacePresenterState& presenterState,
                                          const ExternalSurfacePresenterState* previousPresenterState) override;

    void removeSurfacePresenter(SurfacePresenterId presenterId) override;

protected:
    virtual id getEmbeddedViewForExternalSurface(ExternalSurface& externalSurface) = 0;

private:
    __weak id<SCSnapDrawingMetalSurfacePresenterManagerDelegate> _delegate;
    Ref<MetalGraphicsContext> _graphicsContext;
    Valdi::FlatMap<SurfacePresenterId, CFTypeRef> _presenters;

    id presenterForDisplayId(SurfacePresenterId presenterId) const;
};

}; // namespace snap::drawing
