//
//  GLGraphicsContext.hpp
//  snap_drawing
//
//  Created by Simon Corsin on 5/27/22.
//

#pragma once

#include "snap_drawing/cpp/Drawing/GraphicsContext/GrGraphicsContext.hpp"
#include "snap_drawing/cpp/Drawing/Surface/DrawableSurface.hpp"

#include "include/gpu/ganesh/GrBackendSurface.h"
#include "include/gpu/ganesh/gl/GrGLTypes.h"

#include <EGL/egl.h>
#include <GLES/gl.h>

namespace snap::drawing {

class GLGraphicsContext;

class GLDrawableSurface : public DrawableSurface {
public:
    GLDrawableSurface(const Ref<GLGraphicsContext>& context,
                      const GrBackendTexture& backendTexture,
                      int sampleCount,
                      Valdi::ColorType colorType,
                      bool isOwned);
    ~GLDrawableSurface() override;

    GraphicsContext* getGraphicsContext() override;

    Valdi::Result<DrawableSurfaceCanvas> prepareCanvas() override;

    void flush() override;

    GrGLuint getTextureId() const;
    int getTextureWidth() const;
    int getTextureHeight() const;

    static void flushSurface(SkSurface* surface);

private:
    Ref<GLGraphicsContext> _context;
    sk_sp<SkSurface> _surface;
    GrBackendTexture _backendTexture;
    int _sampleCount;
    SkColorType _colorType;
    bool _owned;
};

class GLGraphicsContext : public GrGraphicsContext {
public:
    GLGraphicsContext(const sk_sp<GrDirectContext>& grContext, const Ref<GrGraphicsContextOptions>& options);
    ~GLGraphicsContext() override;

    Ref<GLDrawableSurface> createSurface(int width, int height, Valdi::ColorType colorType, int sampleCount);

    Ref<GLDrawableSurface> createSurfaceWrappingTexture(GrGLuint textureId,
                                                        int width,
                                                        int height,
                                                        Valdi::ColorType colorType,
                                                        int sampleCount,
                                                        GrGLenum format,
                                                        GrGLenum target);

    static Valdi::Result<Ref<GLGraphicsContext>> create(const Ref<GrGraphicsContextOptions>& options);
};

} // namespace snap::drawing
