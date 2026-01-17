//
//  SCValdiMacOSSurfacePresenterManager.cpp
//  valdi-desktop-apple
//
//  Created by Simon Corsin on 2/22/22.
//

#import "SCValdiMacOSSurfacePresenterManager.h"

#include "valdi/snap_drawing/BridgedView.hpp"

#import "valdi/macos/SCValdiMacOSViewManager.h"
#import "valdi/macos/SCValdiObjCUtils.h"

namespace snap::drawing {

MacOSSurfacePresenterManager::MacOSSurfacePresenterManager(id<SCSnapDrawingMetalSurfacePresenterManagerDelegate> delegate,
                                                       const Ref<MetalGraphicsContext>& graphicsContext): MetalSurfacePresenterManager(delegate, graphicsContext) {}

id MacOSSurfacePresenterManager::getEmbeddedViewForExternalSurface(ExternalSurface& externalSurface) {
    return ValdiMacOS::fromValdiView(dynamic_cast<BridgedView &>(externalSurface).getView());
}

}
