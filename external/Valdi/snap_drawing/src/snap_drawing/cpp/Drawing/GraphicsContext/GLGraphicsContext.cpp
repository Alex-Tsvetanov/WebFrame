//
//  GLGraphicsContext.cpp
//  snap_drawing
//
//  Created by Simon Corsin on 5/27/22.
//

#ifdef SK_GL

#include "include/core/SkColorSpace.h"
#include "include/core/SkSurface.h"
#include "include/gpu/ganesh/SkSurfaceGanesh.h"
#include "include/gpu/ganesh/gl/GrGLBackendSurface.h"
#include "include/gpu/ganesh/gl/GrGLDirectContext.h"
#include "include/gpu/ganesh/gl/GrGLInterface.h"

#include "snap_drawing/cpp/Drawing/GraphicsContext/EGLUtils.hpp"
#include "snap_drawing/cpp/Drawing/GraphicsContext/GLGraphicsContext.hpp"

#include "snap_drawing/cpp/Utils/BitmapUtils.hpp"
#include "valdi_core/cpp/Utils/Format.hpp"

#include <GLES/glext.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES/gl.h>
#include <GLES/glext.h>

namespace snap::drawing {

GLDrawableSurface::GLDrawableSurface(const Ref<GLGraphicsContext>& context,
                                     const GrBackendTexture& backendTexture,
                                     int sampleCount,
                                     Valdi::ColorType colorType,
                                     bool isOwned)
    : _context(context),
      _backendTexture(backendTexture),
      _sampleCount(sampleCount),
      _colorType(toSkiaColorType(colorType)),
      _owned(isOwned) {}

GLDrawableSurface::~GLDrawableSurface() {
    if (_owned) {
        _context->getGrContext().deleteBackendTexture(_backendTexture);
    }
}

GraphicsContext* GLDrawableSurface::getGraphicsContext() {
    return _context.get();
}

Valdi::Result<DrawableSurfaceCanvas> GLDrawableSurface::prepareCanvas() {
    _context->getGrContext().resetContext(kTextureBinding_GrGLBackendState);

    if (_surface == nullptr) {
        SkSurfaceProps surfaceProps;

        _surface = SkSurfaces::WrapBackendTexture(&_context->getGrContext(),
                                                  _backendTexture,
                                                  kBottomLeft_GrSurfaceOrigin,
                                                  _sampleCount,
                                                  _colorType,
                                                  nullptr,
                                                  &surfaceProps);

        if (_surface == nullptr) {
            return Valdi::Error("Failed to create Surface from Backend Texture");
        }
    }

    return DrawableSurfaceCanvas(_surface.get(), getTextureWidth(), getTextureHeight());
}

void GLDrawableSurface::flush() {
    if (_surface != nullptr) {
        GLDrawableSurface::flushSurface(_surface.get());
        _surface = nullptr;
    }
}

void GLDrawableSurface::flushSurface(SkSurface* surface) {
    auto direct = surface->recordingContext() ? surface->recordingContext()->asDirectContext() : nullptr;
    if (!direct) {
        return;
    }

    direct->flushAndSubmit();
}

GrGLuint GLDrawableSurface::getTextureId() const {
    GrGLTextureInfo textureInfo;
    GrBackendTextures::GetGLTextureInfo(_backendTexture, &textureInfo);
    return textureInfo.fID;
}

int GLDrawableSurface::getTextureWidth() const {
    return _backendTexture.width();
}

int GLDrawableSurface::getTextureHeight() const {
    return _backendTexture.height();
}

GLGraphicsContext::GLGraphicsContext(const sk_sp<GrDirectContext>& grContext,
                                     const Ref<GrGraphicsContextOptions>& options)
    : GrGraphicsContext(grContext, options) {}

GLGraphicsContext::~GLGraphicsContext() = default;

Valdi::Result<Ref<GLGraphicsContext>> GLGraphicsContext::create(const Ref<GrGraphicsContextOptions>& options) {
    auto grContext = GrDirectContexts::MakeGL(nullptr, options->getGrContextOptions());
    if (grContext == nullptr) {
        return Valdi::Error("Failed to create GLGraphicsContext");
    }

    return Valdi::makeShared<GLGraphicsContext>(grContext, options);
}

Ref<GLDrawableSurface> GLGraphicsContext::createSurface(int width,
                                                        int height,
                                                        Valdi::ColorType colorType,
                                                        int sampleCount) {
    auto backendTexture = getGrContext().createBackendTexture(
        width, height, toSkiaColorType(colorType), skgpu::Mipmapped::kYes, GrRenderable::kYes);

    return Valdi::makeShared<GLDrawableSurface>(
        Valdi::strongSmallRef(this), backendTexture, sampleCount, colorType, true);
}

Ref<GLDrawableSurface> GLGraphicsContext::createSurfaceWrappingTexture(GrGLuint textureId,
                                                                       int width,
                                                                       int height,
                                                                       Valdi::ColorType colorType,
                                                                       int sampleCount,
                                                                       GrGLenum format,
                                                                       GrGLenum target) {
    GrGLTextureInfo textureInfo;
    textureInfo.fFormat = format;
    textureInfo.fTarget = target;
    textureInfo.fID = textureId;

    auto backendTexture = GrBackendTextures::MakeGL(width, height, skgpu::Mipmapped::kNo, textureInfo);

    return Valdi::makeShared<GLDrawableSurface>(
        Valdi::strongSmallRef(this), backendTexture, sampleCount, colorType, false);
}

} // namespace snap::drawing

#endif
