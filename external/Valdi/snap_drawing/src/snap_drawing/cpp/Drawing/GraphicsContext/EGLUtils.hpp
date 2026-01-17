//
//  EGLUtils.hpp
//  snap_drawing-macos
//
//  Created by Simon Corsin on 6/2/22.
//

#pragma once

#include <EGL/egl.h>
#include <GLES/gl.h>

#include "utils/base/NonCopyable.hpp"
#include "valdi_core/cpp/Utils/Error.hpp"

#include <iostream>

namespace snap::drawing {

class EGLContextWrapper {
public:
    EGLContextWrapper(EGLDisplay display, EGLSurface drawSurface, EGLSurface readSurface, EGLContext context);
    EGLContextWrapper();

    bool makeCurrent();
    bool releaseCurrent();

    EGLDisplay getDisplay() const;
    void setDisplay(EGLDisplay display);

    EGLSurface getDrawSurface() const;
    void setDrawSurface(EGLSurface drawSurface);

    EGLSurface getReadSurface() const;
    void setReadSurface(EGLSurface readSurface);

    EGLContext getContext() const;
    void setContext(EGLContext context);

    static EGLContextWrapper current();

    static Valdi::Error getLastError(const char* errorMessagePrefix);

private:
    EGLDisplay _display;
    EGLSurface _drawSurface;
    EGLSurface _readSurface;
    EGLContext _context;
};

std::ostream& operator<<(std::ostream& os, const EGLContextWrapper& ctx);

class EGLContextScope : public snap::NonCopyable {
public:
    EGLContextScope();
    EGLContextScope(EGLContextScope&& other) noexcept;
    explicit EGLContextScope(EGLContextWrapper context);
    EGLContextScope(EGLContextWrapper oldContext, EGLContextWrapper newContext);

    ~EGLContextScope();

    EGLContextScope& operator=(EGLContextScope&& other) noexcept;

    explicit operator bool() const noexcept;

private:
    EGLContextWrapper _oldContext;
    bool _valid = false;
};

} // namespace snap::drawing
