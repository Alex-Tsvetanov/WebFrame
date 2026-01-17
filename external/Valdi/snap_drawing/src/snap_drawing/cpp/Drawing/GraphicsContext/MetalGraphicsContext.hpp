//
//  MetalGraphicsContext.hpp
//  snap_drawing
//
//  Created by Simon Corsin on 1/20/22.
//

#pragma once

#include "snap_drawing/cpp/Drawing/GraphicsContext/GrGraphicsContext.hpp"
#include "snap_drawing/cpp/Drawing/Surface/DrawableSurface.hpp"

#include "CoreFoundation/CoreFoundation.h"
#include "valdi_core/cpp/Interfaces/IBitmap.hpp"

namespace snap::drawing {

struct MetalLayerSize {
    double width;
    double height;

    constexpr MetalLayerSize(double width, double height) : width(width), height(height) {}
};

class MetalGraphicsContext;

class MetalDrawableSurface : public DrawableSurface {
public:
    MetalDrawableSurface(const Ref<MetalGraphicsContext>& context, CFTypeRef mtlLayer, int sampleCount);
    ~MetalDrawableSurface() override;

    GraphicsContext* getGraphicsContext() override;

    Valdi::Result<DrawableSurfaceCanvas> prepareCanvas() override;

    void flush() override;

    CFTypeRef getMTLLayer() const;

private:
    Ref<MetalGraphicsContext> _context;
    sk_sp<SkSurface> _surface;
    CFTypeRef _mtlLayer;
    CFTypeRef _mtlDrawableHandle = nullptr;
    int _sampleCount;
};

class MetalGraphicsContext : public GrGraphicsContext {
public:
    MetalGraphicsContext(CFTypeRef mtlDevice, CFTypeRef mtlCommandQueue, const Ref<GrGraphicsContextOptions>& options);
    ~MetalGraphicsContext() override;

    Ref<DrawableSurface> createSurfaceForMetalLayer(CFTypeRef mtlLayer, int sampleCount);

    void commit() override;

    CFTypeRef getMTLDevice() const;
    CFTypeRef getMTLCommandQueue() const;

private:
    CFTypeRef _mtlDevice;
    CFTypeRef _mtlCommandQueue;
    CFTypeRef _mtlCommandBuffer = nullptr;

    friend MetalDrawableSurface;

    void presentMetalDrawable(CFTypeRef mtlDrawableHandle);

    // These methods are not directly implemented in the cpp files, the implementation should live in a .mm file
    static CFTypeRef createCommandBuffer(CFTypeRef mtlCommandQueue);
    static void presentMetalDrawable(CFTypeRef mtlCommandBuffer, CFTypeRef mtlDrawableHandle);
    static void commitCommandBuffer(CFTypeRef mtlCommandBuffer);
    static MetalLayerSize getMetalLayerSize(CFTypeRef mtlLayer);

    static sk_sp<GrDirectContext> makeContext(void* mtlDevice, void* mtlCommandQueue, const GrContextOptions& options);
    static sk_sp<SkSurface> makeSurface(GrDirectContext* grContext,
                                        const void* mtlLayer,
                                        int sampleCount,
                                        SkColorType colorType,
                                        const void** mtlDrawableHandle);
};

} // namespace snap::drawing
