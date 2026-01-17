//
//  SCValdiMetalSurfacePresenterManager.cpp
//  valdi-desktop-apple
//
//  Created by Simon Corsin on 2/22/22.
//

#include "snap_drawing/apple/MetalSurfacePresenterManager.h"
#include "snap_drawing/apple/CoreGraphicsUtils.hpp"
#include "utils/platform/TargetPlatform.hpp"

#import <QuartzCore/CAMetalLayer.h>

namespace snap::drawing {

MetalSurfacePresenterManager::MetalSurfacePresenterManager(id<SCSnapDrawingMetalSurfacePresenterManagerDelegate> delegate,
                                                           const Ref<MetalGraphicsContext> &graphicsContext): _delegate(delegate), _graphicsContext(graphicsContext) {
    
}

MetalSurfacePresenterManager::~MetalSurfacePresenterManager() {
    _delegate = nil;
    for (const auto &it: _presenters) {
        CFRelease(it.second);
    }
}

Ref<DrawableSurface> MetalSurfacePresenterManager::createPresenterWithDrawableSurface(SurfacePresenterId presenterId, size_t zIndex) {
    if (@available(iOS 13.0, macOS 10.11, *)) {
        CAMetalLayer *metalLayer;
        id presenter = [_delegate createMetalPresenter:&metalLayer];
        if (!presenter) {
            return nullptr;
        }

        metalLayer.device = (__bridge  id)_graphicsContext->getMTLDevice();
        metalLayer.pixelFormat = MTLPixelFormatBGRA8Unorm;
#if __APPLE__ && !SC_IOS
        metalLayer.displaySyncEnabled = YES;
#endif
        metalLayer.framebufferOnly = YES;
        metalLayer.opaque = NO;

        auto surface = _graphicsContext->createSurfaceForMetalLayer((__bridge CFTypeRef)metalLayer, 1);

        _presenters[presenterId] = CFBridgingRetain(presenter);

        setSurfacePresenterZIndex(presenterId, zIndex);

        return surface;
    } else {
        return nullptr;
    }
}

void MetalSurfacePresenterManager::createPresenterForExternalSurface(SurfacePresenterId presenterId, size_t zIndex, ExternalSurface& externalSurface) {
    id embeddedView = getEmbeddedViewForExternalSurface(externalSurface);
    id presenter = [_delegate createEmbeddedPresenterForView:embeddedView];
    if (!presenter) {
        return;
    }

    _presenters[presenterId] = CFBridgingRetain(presenter);
    
    setSurfacePresenterZIndex(presenterId, zIndex);
}

void MetalSurfacePresenterManager::setSurfacePresenterZIndex(SurfacePresenterId presenterId, size_t zIndex) {
    id presenter = presenterForDisplayId(presenterId);
    [_delegate setZIndex:zIndex forPresenter:presenter];
}

void MetalSurfacePresenterManager::setExternalSurfacePresenterState(SurfacePresenterId presenterId,
                                                                    const ExternalSurfacePresenterState& presenterState,
                                                                    const ExternalSurfacePresenterState* previousPresenterState) {
    id presenter = presenterForDisplayId(presenterId);
    CGRect frame = CGRectMake(presenterState.frame.x(), presenterState.frame.y(), presenterState.frame.width(), presenterState.frame.height());
    auto transform = snap::drawing::caTransform3DFromMatrix(presenterState.transform);
    
    BOOL clipHasChanged = NO;
    CGPathRef clipPathRef = nil;
    
    if (previousPresenterState == nullptr || presenterState.clipPath != previousPresenterState->clipPath) {
        clipHasChanged = YES;
        
        if (!presenterState.clipPath.isEmpty()) {
            clipPathRef = snap::drawing::cgPathFromPath(presenterState.clipPath);
        }
    }

    [_delegate setFrame:frame
              transform:transform
                opacity:presenterState.opacity
               clipPath:clipPathRef
         clipHasChanged:clipHasChanged
   forEmbeddedPresenter:presenter];

    if (clipPathRef) {
        CGPathRelease(clipPathRef);
    }
}

void MetalSurfacePresenterManager::removeSurfacePresenter(SurfacePresenterId presenterId) {
    auto it = _presenters.find(presenterId);
    if (it == _presenters.end()) {
        return;
    }

    id presenter = CFBridgingRelease(it->second);

    _presenters.erase(it);

    [_delegate removePresenter:presenter];
}

id MetalSurfacePresenterManager::presenterForDisplayId(SurfacePresenterId presenterId) const {
    const auto &it = _presenters.find(presenterId);
    if (it == _presenters.end()) {
        return nil;
    }

    return (__bridge  id)it->second;
}

}
