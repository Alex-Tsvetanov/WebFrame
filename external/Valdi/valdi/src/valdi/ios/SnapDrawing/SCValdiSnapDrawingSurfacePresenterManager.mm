//
//  SCValdiSnapDrawingSurfacePresenterManager.cpp
//  valdi-desktop-apple
//
//  Created by Simon Corsin on 2/22/22.
//

#import "SCValdiSnapDrawingSurfacePresenterManager.h"

#include "valdi/snap_drawing/BridgedView.hpp"
#include "valdi/ios/CPPBindings/UIViewHolder.h"

namespace snap::drawing {

IOSSurfacePresenterManager::IOSSurfacePresenterManager(id<SCSnapDrawingMetalSurfacePresenterManagerDelegate> delegate,
                                                       const Ref<MetalGraphicsContext>& graphicsContext): MetalSurfacePresenterManager(delegate, graphicsContext) {}

id IOSSurfacePresenterManager::getEmbeddedViewForExternalSurface(ExternalSurface& externalSurface) {
    return ValdiIOS::UIViewHolder::uiViewFromRef(dynamic_cast<BridgedView &>(externalSurface).getView());
}

}
