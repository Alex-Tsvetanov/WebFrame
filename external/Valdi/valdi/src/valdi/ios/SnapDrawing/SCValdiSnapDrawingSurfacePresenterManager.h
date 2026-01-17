//
//  SCValdiMacOSSurfacePresenterManager.hpp
//  valdi-desktop-apple
//
//  Created by Simon Corsin on 2/22/22.
//

#ifdef __cplusplus

#pragma once

#import "snap_drawing/apple/MetalSurfacePresenterManager.h"

namespace snap::drawing {

class IOSSurfacePresenterManager : public MetalSurfacePresenterManager {
public:
    IOSSurfacePresenterManager(id<SCSnapDrawingMetalSurfacePresenterManagerDelegate> delegate,
                               const Ref<MetalGraphicsContext>& graphicsContext);

protected:
    id getEmbeddedViewForExternalSurface(ExternalSurface& externalSurface) override;
};

}; // namespace snap::drawing

#endif