#pragma once

#include "djinni/jni/djinni_support.hpp"
#include "valdi_core/bindings/UndefExceptions_jni.hpp"
#include "valdi_core/cpp/Utils/Bytes.hpp"
#include "valdi_core/jni/JavaUtils.hpp"

namespace ValdiAndroid {

struct BytesTranslator {
    using CppType = Valdi::BytesView;
    using JniType = jobject;

    static CppType toCpp(JNIEnv* jniEnv, JniType o) {
        return toByteArray(JavaEnv(), reinterpret_cast<jbyteArray>(o));
    }

    static ::djinni::LocalRef<JniType> fromCpp(JNIEnv* jniEnv, const CppType& c) {
        auto ref = toJavaObject(JavaEnv(), c).releaseObject();
        return ::djinni::LocalRef<JniType>(ref);
    }

    using Boxed = BytesTranslator;
};

} // namespace ValdiAndroid
