//
//  EGLUtils.cpp
//  snap_drawing-macos
//
//  Created by Simon Corsin on 6/2/22.
//

#if __ANDROID__

#include "snap_drawing/cpp/Drawing/GraphicsContext/EGLUtils.hpp"
#include "valdi_core/cpp/Utils/Format.hpp"
#include "valdi_core/cpp/Utils/StringCache.hpp"

namespace snap::drawing {

EGLContextWrapper::EGLContextWrapper(EGLDisplay display,
                                     EGLSurface drawSurface,
                                     EGLSurface readSurface,
                                     EGLContext context)
    : _display(display), _drawSurface(drawSurface), _readSurface(readSurface), _context(context) {}

EGLContextWrapper::EGLContextWrapper()
    : _display(EGL_NO_DISPLAY), _drawSurface(EGL_NO_SURFACE), _readSurface(EGL_NO_SURFACE), _context(EGL_NO_CONTEXT) {}

Valdi::Error EGLContextWrapper::getLastError(const char* errorMessagePrefix) {
    auto errorCode = eglGetError();
    // TODO(simon): Translate error code to string?
    return Valdi::Error(STRING_FORMAT("{}: EGL error code {}", errorMessagePrefix, static_cast<int>(errorCode)));
}

bool EGLContextWrapper::makeCurrent() {
    return eglMakeCurrent(_display, _drawSurface, _readSurface, _context) != EGL_FALSE;
}

bool EGLContextWrapper::releaseCurrent() {
    return EGLContextWrapper(_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT).makeCurrent();
}

EGLDisplay EGLContextWrapper::getDisplay() const {
    return _display;
}

void EGLContextWrapper::setDisplay(EGLDisplay display) {
    _display = display;
}

EGLSurface EGLContextWrapper::getDrawSurface() const {
    return _drawSurface;
}

void EGLContextWrapper::setDrawSurface(EGLSurface drawSurface) {
    _drawSurface = drawSurface;
}

EGLSurface EGLContextWrapper::getReadSurface() const {
    return _readSurface;
}

void EGLContextWrapper::setReadSurface(EGLSurface readSurface) {
    _readSurface = readSurface;
}

EGLContext EGLContextWrapper::getContext() const {
    return _context;
}

void EGLContextWrapper::setContext(EGLContext context) {
    _context = context;
}

EGLContextWrapper EGLContextWrapper::current() {
    return EGLContextWrapper(
        eglGetCurrentDisplay(), eglGetCurrentSurface(EGL_DRAW), eglGetCurrentSurface(EGL_READ), eglGetCurrentContext());
}

std::ostream& operator<<(std::ostream& os, const EGLContextWrapper& ctx) {
    return os << "EGLDisplay: " << reinterpret_cast<void*>(ctx.getDisplay())
              << ", DrawSurface: " << reinterpret_cast<void*>(ctx.getDrawSurface())
              << ", ReadSurface: " << reinterpret_cast<void*>(ctx.getReadSurface())
              << ", Context: " << reinterpret_cast<void*>(ctx.getContext());
}

EGLContextScope::EGLContextScope() = default;
EGLContextScope::EGLContextScope(EGLContextScope&& other) noexcept
    : _oldContext(other._oldContext), _valid(other._valid) {
    other._oldContext = EGLContextWrapper();
    other._valid = false;
}
EGLContextScope::EGLContextScope(EGLContextWrapper context) : EGLContextScope(EGLContextWrapper::current(), context) {}

EGLContextScope::EGLContextScope(EGLContextWrapper oldContext, EGLContextWrapper context)
    : _oldContext(oldContext), _valid(context.makeCurrent()) {}

EGLContextScope::~EGLContextScope() {
    if (_valid) {
        _oldContext.makeCurrent();
    }
}

EGLContextScope& EGLContextScope::operator=(EGLContextScope&& other) noexcept {
    if (this != &other) {
        _oldContext = other._oldContext;
        _valid = other._valid;

        other._oldContext = EGLContextWrapper();
        other._valid = false;
    }

    return *this;
}

EGLContextScope::operator bool() const noexcept {
    return _valid;
}

} // namespace snap::drawing

#endif
