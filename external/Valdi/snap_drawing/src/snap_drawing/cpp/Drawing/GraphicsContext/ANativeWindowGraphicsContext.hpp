//
//  ANativeWindowGraphicsContext.hpp
//  snap_drawing
//
//  Created by Simon Corsin on 1/20/22.
//

#pragma once

#include <android/native_window_jni.h>

#include "snap_drawing/cpp/Drawing/GraphicsContext/GLGraphicsContext.hpp"
#include "snap_drawing/cpp/Drawing/GraphicsContext/GrGraphicsContext.hpp"
#include "snap_drawing/cpp/Drawing/Surface/DrawableSurface.hpp"
#include "valdi_core/cpp/Utils/Mutex.hpp"
#include "valdi_core/cpp/Utils/Result.hpp"

#include <optional>

namespace snap::drawing {

struct DisplayParams {
    DisplayParams() = default;

    int mSAASampleCount = 1;
    bool disableVsync = false;
};

class EGLContextWrapper;

class ANativeWindowDrawableSurface;

class ANativeWindowGraphicsContext : public GraphicsContext {
public:
    ANativeWindowGraphicsContext(const DisplayParams& displayParams, const Ref<GrGraphicsContextOptions>& options);
    ~ANativeWindowGraphicsContext() override;

    void setResourceCacheLimit(size_t resourceCacheLimitBytes) override;

    void performCleanup(bool shouldPurgeScratchResources, std::chrono::seconds secondsNotUsed) override;

    Ref<DrawableSurface> createSurfaceForNativeWindow(ANativeWindow* nativeWindow);

    int64_t getMaxRenderTargetSize();

    static Ref<ANativeWindowGraphicsContext> create(const DisplayParams& displayParams,
                                                    const Ref<GrGraphicsContextOptions>& options);

private:
    struct GLObjects {
        EGLDisplay eGLDisplay;
        EGLContext eGLContext;
        EGLSurface eGLDefaultSurface = EGL_NO_SURFACE;
        EGLConfig eGLConfig = nullptr;
        int sampleCount = 0;
        int stencilBits = 0;
        sk_sp<GrDirectContext> grContext;

        GLObjects();
    };

    Valdi::Mutex _mutex;
    DisplayParams _displayParams;
    Ref<GrGraphicsContextOptions> _options;
    std::optional<GLObjects> _glObjects;

    Valdi::Result<GLObjects> getGLObjects();
    static Valdi::Error onGLObjectsError(GLObjects& glObjects, const char* message);
    static Valdi::Error onBindAPIError(GLObjects& glObjects);
    static Valdi::Error onGetDefaultDisplayError(GLObjects& glObjects);
    static Valdi::Error onInitializeError(GLObjects& glObjects);
    static Valdi::Error onChooseConfigError(GLObjects& glObjects);
    static Valdi::Error onCreateContextError(GLObjects& glObjects);
    static Valdi::Error onCreateDefaultSurfaceError(GLObjects& glObjects);
    static Valdi::Error onMakeCurrentError(GLObjects& glObjects);
    static Valdi::Error onGrContextError(GLObjects& glObjects);

    friend ANativeWindowDrawableSurface;

    std::optional<EGLContextWrapper> makeContextCurrent();
};

} // namespace snap::drawing
