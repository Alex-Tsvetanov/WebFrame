#pragma once

#include "valdi_core/jni/JavaObject.hpp"

namespace ValdiAndroid {

class JavaException : public JavaObject {
public:
    JavaException(JavaEnv vm, djinni::LocalRef<jobject> exception);
    ~JavaException();

    void handleFatal(JNIEnv* env, std::optional<std::string> additionalInfo) const;

    std::string getMessage() const;
};

} // namespace ValdiAndroid
