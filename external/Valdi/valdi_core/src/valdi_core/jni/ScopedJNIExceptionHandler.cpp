#include "valdi_core/jni/ScopedJNIExceptionHandler.hpp"

namespace ValdiAndroid {

ScopedJNIExceptionHandler::ScopedJNIExceptionHandler()
    : _previousExceptionHandler(JavaEnv::getExceptionHandlerForCurrentThread()) {
    JavaEnv::setExceptionHandlerForCurrentThread(this);
}

ScopedJNIExceptionHandler::~ScopedJNIExceptionHandler() {
    JavaEnv::setExceptionHandlerForCurrentThread(_previousExceptionHandler);
}

void ScopedJNIExceptionHandler::onException(JavaException exception) {
    _exception = std::make_optional<JavaException>(std::move(exception));
}

ScopedJNIExceptionHandler::operator bool() const noexcept {
    return !_exception.has_value();
}

const std::optional<JavaException>& ScopedJNIExceptionHandler::getException() const {
    return _exception;
}

void ScopedJNIExceptionHandler::clear() {
    _exception = std::nullopt;
}

} // namespace ValdiAndroid
