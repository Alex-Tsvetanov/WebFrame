//
//  MacOSSnapDrawingRuntime.h
//  valdi-desktop-apple
//
//  Created by Simon Corsin on 7/27/22.
//

#pragma once

#include "snap_drawing/apple/Drawing/CVDisplayLinkFrameScheduler.hpp"
#include "valdi/snap_drawing/Runtime.hpp"

namespace snap::drawing {
class MetalGraphicsContext;

}
namespace ValdiMacOS {

class MacOSSnapDrawingRuntime : public snap::drawing::Runtime {
public:
    MacOSSnapDrawingRuntime(const Valdi::Ref<Valdi::IDiskCache>& diskCache,
                            Valdi::IViewManager& hostViewManager,
                            Valdi::ILogger& logger,
                            const Valdi::Ref<Valdi::DispatchQueue>& workerQueue,
                            uint64_t maxCacheSizeInBytes);

    ~MacOSSnapDrawingRuntime() override;

    void setActiveDisplay(CGDirectDisplayID displayId);

    snap::drawing::Ref<snap::drawing::MetalGraphicsContext> getMetalGraphicsContext() const;
};

} // namespace ValdiMacOS
