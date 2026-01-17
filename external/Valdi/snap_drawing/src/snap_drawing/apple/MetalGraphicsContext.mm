#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

#import "snap_drawing/cpp/Drawing/GraphicsContext/MetalGraphicsContext.hpp"

#include "include/gpu/ganesh/GrDirectContext.h"
#include "include/gpu/ganesh/mtl/SkSurfaceMetal.h"
#include "include/gpu/ganesh/mtl/GrMtlDirectContext.h"
#include "include/gpu/ganesh/mtl/GrMtlBackendContext.h"
#include "include/core/SkSurface.h"
#include "include/core/SkColorSpace.h"

namespace snap::drawing {

void MetalGraphicsContext::presentMetalDrawable(CFTypeRef mtlCommandBuffer, CFTypeRef mtlDrawableHandle) {
    id<CAMetalDrawable> currentDrawable = (__bridge id<CAMetalDrawable>)mtlDrawableHandle;
    id<MTLCommandBuffer> commandBuffer = (__bridge id<MTLCommandBuffer>)mtlCommandBuffer;

    if (currentDrawable) {
        [commandBuffer presentDrawable:currentDrawable];
    }
}

CFTypeRef MetalGraphicsContext::createCommandBuffer(CFTypeRef mtlCommandQueue) {
    id<MTLCommandBuffer> commandBuffer = [(__bridge id<MTLCommandQueue>)mtlCommandQueue commandBuffer];
    CFTypeRef rawCommandBuffer = (__bridge CFTypeRef)(commandBuffer);

    CFRetain(rawCommandBuffer);
    return rawCommandBuffer;
}

void MetalGraphicsContext::commitCommandBuffer(CFTypeRef mtlCommandBuffer) {
    id<MTLCommandBuffer> commandBuffer = (__bridge id<MTLCommandBuffer>)mtlCommandBuffer;
    [commandBuffer commit];
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunguarded-availability-new"

snap::drawing::MetalLayerSize MetalGraphicsContext::getMetalLayerSize(CFTypeRef mtlLayer) {
    CAMetalLayer *metalLayer = (__bridge CAMetalLayer *)mtlLayer;

    CGSize drawableSize = metalLayer.drawableSize;

    return snap::drawing::MetalLayerSize(drawableSize.width, drawableSize.height);
}

#pragma clang diagnostic pop

sk_sp<GrDirectContext> MetalGraphicsContext::makeContext(void *mtlDevice, void *mtlCommandQueue, const GrContextOptions &options) {
    GrMtlBackendContext mtlBackendContext;
    mtlBackendContext.fDevice.retain(mtlDevice);
    mtlBackendContext.fQueue.retain(mtlCommandQueue);

    return GrDirectContexts::MakeMetal(mtlBackendContext, options);
}

sk_sp<SkSurface> MetalGraphicsContext::makeSurface(GrDirectContext *grContext, const void *mtlLayer, int sampleCount, SkColorType colorType, const void **mtlDrawableHandle) {
    return SkSurfaces::WrapCAMetalLayer(grContext, mtlLayer, kTopLeft_GrSurfaceOrigin, sampleCount, colorType, nullptr, nullptr, mtlDrawableHandle);
}

}
