//
//  SCValdiSnapDrawingRuntime.m
//  valdi-ios
//
//  Created by Simon Corsin on 8/30/22.
//

#import "SCValdiSnapDrawingRuntime.h"
#import <Metal/Metal.h>

#import "valdi/snap_drawing/Runtime.hpp"
#import "valdi/snap_drawing/Graphics/ShaderCache.hpp"

#import "snap_drawing/cpp/Drawing/GraphicsContext/MetalGraphicsContext.hpp"
#import "snap_drawing/cpp/Drawing/DrawLooper.hpp"
#import "snap_drawing/cpp/Touches/GesturesConfiguration.hpp"
#import "snap_drawing/apple/Drawing/CADisplayLinkFrameScheduler.h"

namespace ValdiIOS {

class IOSSnapDrawingRuntime : public snap::drawing::Runtime {
public:
    IOSSnapDrawingRuntime(const Valdi::Ref<Valdi::IDiskCache>& diskCache,
                            Valdi::IViewManager& hostViewManager,
                            Valdi::ILogger& logger,
                            const Valdi::Ref<Valdi::DispatchQueue>& workerQueue,
                            uint64_t maxCacheSizeInBytes):
    snap::drawing::Runtime(
        Valdi::makeShared<snap::drawing::CADisplayLinkFrameScheduler>(logger),
        snap::drawing::GesturesConfiguration::getDefault(),
        diskCache,
        &hostViewManager,
        logger,
        workerQueue,
        maxCacheSizeInBytes) {

        id<MTLDevice> device = MTLCreateSystemDefaultDevice();
        id<MTLCommandQueue> commandQueue = [device newCommandQueue];

        auto options = Valdi::makeShared<snap::drawing::GrGraphicsContextOptions>(getShaderCache());

        setGraphicsContext(Valdi::makeShared<snap::drawing::MetalGraphicsContext>((__bridge CFTypeRef)device,
                                                                                     (__bridge CFTypeRef)commandQueue,
                                                                                     options));

        initializeViewManager(Valdi::PlatformTypeIOS);
    }

    ~IOSSnapDrawingRuntime() override = default;

private:
    snap::drawing::Ref<snap::drawing::MetalGraphicsContext> _graphicsContext;
};
};

@implementation SCValdiSnapDrawingRuntime {
    Valdi::Ref<ValdiIOS::IOSSnapDrawingRuntime> _instance;
}

- (instancetype)initWithDiskCache:(Valdi::IDiskCache *)diskCache
                  hostViewManager:(Valdi::IViewManager *)hostViewManager
                           logger:(Valdi::ILogger *)logger
                      workerQueue:(Valdi::DispatchQueue *)workerQueue
               assetLoaderManager:(Valdi::AssetLoaderManager*)assetLoaderManager
              maxCacheSizeInBytes:(uint64_t)maxCacheSizeInBytes
{
    self = [super init];

    if (self) {
        _instance = Valdi::makeShared<ValdiIOS::IOSSnapDrawingRuntime>(Valdi::strongSmallRef(diskCache),
                                                                             *hostViewManager,
                                                                             *logger,
                                                                             Valdi::strongSmallRef(workerQueue),
                                                                             maxCacheSizeInBytes);
        _instance->registerAssetLoaders(*assetLoaderManager);
    }

    return self;
}

- (void)dealloc
{
    _instance = nullptr;
}

- (void)onApplicationEnteringBackground
{
    _instance->getDrawLooper()->onApplicationEnteringBackground();
}

- (void)onApplicationEnteringForeground
{
    _instance->getDrawLooper()->onApplicationEnteringForeground();
}

- (void)onApplicationIsInLowMemory
{
    _instance->getDrawLooper()->onApplicationIsInLowMemory();
}

- (void *)handle
{
    return Valdi::unsafeBridgeCast(_instance.get());
}

@end
