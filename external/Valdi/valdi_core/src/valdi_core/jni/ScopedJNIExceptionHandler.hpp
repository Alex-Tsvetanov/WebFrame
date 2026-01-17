#pragma once

#include "valdi_core/jni/JavaEnv.hpp"
#include "valdi_core/jni/JavaException.hpp"
#include <optional>

namespace ValdiAndroid {

class ScopedJNIExceptionHandler : public JNIExceptionHandler {
public:
    ScopedJNIExceptionHandler();
    ~ScopedJNIExceptionHandler() final;

    void onException(JavaException exception) final;

    explicit operator bool() const noexcept;

    const std::optional<JavaException>& getException() const;

    void clear();

private:
    JNIExceptionHandler* _previousExceptionHandler;
    std::optional<JavaException> _exception;
};

} // namespace ValdiAndroid
