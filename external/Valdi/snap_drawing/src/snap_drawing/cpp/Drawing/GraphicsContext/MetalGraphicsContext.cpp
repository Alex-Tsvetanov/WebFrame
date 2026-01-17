//
//  MetalGraphicsContext.cpp
//  snap_drawing
//
//  Created by Simon Corsin on 1/20/22.
//

#ifdef SK_METAL

#include "snap_drawing/cpp/Drawing/GraphicsContext/MetalGraphicsContext.hpp"

#include "include/core/SkSurface.h"
#include "include/gpu/ganesh/SkSurfaceGanesh.h"

namespace snap::drawing {

MetalDrawableSurface::MetalDrawableSurface(const Ref<MetalGraphicsContext>& context,
                                           CFTypeRef mtlLayer,
                                           int sampleCount)
    : _context(context), _mtlLayer(CFRetain(mtlLayer)), _sampleCount(sampleCount) {}

MetalDrawableSurface::~MetalDrawableSurface() {
    if (_mtlDrawableHandle != nullptr) {
        CFRelease(_mtlDrawableHandle);
    }

    CFRelease(_mtlLayer);
}

GraphicsContext* MetalDrawableSurface::getGraphicsContext() {
    return _context.get();
}

Valdi::Result<DrawableSurfaceCanvas> MetalDrawableSurface::prepareCanvas() {
    if (_surface == nullptr) {
        _surface = MetalGraphicsContext::makeSurface(&_context->getGrContext(),
                                                     const_cast<void*>(_mtlLayer),
                                                     _sampleCount,
                                                     kBGRA_8888_SkColorType,
                                                     &_mtlDrawableHandle);

        if (_surface == nullptr) {
            return Valdi::Error("Failed to create Metal surface");
        }
    }

    auto size = MetalGraphicsContext::getMetalLayerSize(_mtlLayer);

    auto width = static_cast<int>(size.width);
    auto height = static_cast<int>(size.height);

    return DrawableSurfaceCanvas(_surface.get(), width, height);
}

void MetalDrawableSurface::flush() {
    if (_surface == nullptr) {
        return;
    }

    skgpu::ganesh::FlushAndSubmit(_surface);
    _surface = nullptr;

    CFTypeRef mtlDrawableHandle = _mtlDrawableHandle;

    _mtlDrawableHandle = nullptr;

    if (mtlDrawableHandle != nullptr) {
        _context->presentMetalDrawable(mtlDrawableHandle);
        CFRelease(mtlDrawableHandle);
    }
}

MetalGraphicsContext::MetalGraphicsContext(CFTypeRef mtlDevice,
                                           CFTypeRef mtlCommandQueue,
                                           const Ref<GrGraphicsContextOptions>& options)
    : GrGraphicsContext(MetalGraphicsContext::makeContext(const_cast<void*>(mtlDevice),
                                                          const_cast<void*>(mtlCommandQueue),
                                                          options->getGrContextOptions()),
                        options),
      _mtlDevice(CFRetain(mtlDevice)),
      _mtlCommandQueue(CFRetain(mtlCommandQueue)) {}

MetalGraphicsContext::~MetalGraphicsContext() {
    if (_mtlCommandBuffer != nullptr) {
        CFRelease(_mtlCommandBuffer);
    }

    CFRelease(_mtlCommandQueue);
    CFRelease(_mtlDevice);
}

void MetalGraphicsContext::commit() {
    CFTypeRef mtlCommandBuffer = _mtlCommandBuffer;
    if (mtlCommandBuffer != nullptr) {
        _mtlCommandBuffer = nullptr;
        MetalGraphicsContext::commitCommandBuffer(mtlCommandBuffer);
        CFRelease(mtlCommandBuffer);
    }
}

void MetalGraphicsContext::presentMetalDrawable(CFTypeRef mtlDrawableHandle) {
    if (_mtlCommandBuffer == nullptr) {
        _mtlCommandBuffer = MetalGraphicsContext::createCommandBuffer(_mtlCommandQueue);
    }

    MetalGraphicsContext::presentMetalDrawable(_mtlCommandBuffer, mtlDrawableHandle);
}

Ref<DrawableSurface> MetalGraphicsContext::createSurfaceForMetalLayer(CFTypeRef mtlLayer, int sampleCount) {
    return Valdi::makeShared<MetalDrawableSurface>(Valdi::strongSmallRef(this), mtlLayer, sampleCount);
}

CFTypeRef MetalGraphicsContext::getMTLDevice() const {
    return _mtlDevice;
}

CFTypeRef MetalGraphicsContext::getMTLCommandQueue() const {
    return _mtlCommandQueue;
}

} // namespace snap::drawing

#endif
