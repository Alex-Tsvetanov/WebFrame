#pragma once

#include "djinni/jni/djinni_support.hpp"
#include "valdi_core/jni/JavaUtils.hpp"

namespace ValdiAndroid {

struct ResultTranslator {
    using CppType = Valdi::PlatformResult;
    using JniType = jobject;

    static CppType toCpp(JNIEnv* jniEnv, JniType o) {
        return platformResultFromJavaResult(JavaEnv(), o);
    }

    static ::djinni::LocalRef<JniType> fromCpp(JNIEnv* jniEnv, const CppType& c) {
        auto ref = javaResultFromPlatformResult(JavaEnv(), c).releaseObject();
        return ::djinni::LocalRef<JniType>(ref);
    }

    using Boxed = Valdi::PlatformResult;
};

} // namespace ValdiAndroid
