#pragma once

#include "djinni/jni/djinni_support.hpp"
#include "valdi_core/bindings/UndefExceptions_jni.hpp"
#include "valdi_core/cpp/Utils/Value.hpp"
#include "valdi_core/jni/JavaUtils.hpp"

namespace ValdiAndroid {

struct ValueTranslator {
    using CppType = Valdi::Value;
    using JniType = jobject;

    static CppType toCpp(JNIEnv* jniEnv, JniType o) {
        return toValue(JavaEnv(), ::djinni::LocalRef<JniType>(o));
    }

    static ::djinni::LocalRef<JniType> fromCpp(JNIEnv* jniEnv, const CppType& c) {
        auto ref = toJavaObject(JavaEnv(), c).releaseObject();
        return ::djinni::LocalRef<JniType>(ref);
    }

    using Boxed = Valdi::Value;
};

} // namespace ValdiAndroid
