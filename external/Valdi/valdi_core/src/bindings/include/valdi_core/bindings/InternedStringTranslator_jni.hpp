#pragma once

#include "djinni/jni/djinni_support.hpp"
#include "valdi_core/bindings/UndefExceptions_jni.hpp"
#include "valdi_core/cpp/Utils/StringBox.hpp"
#include "valdi_core/jni/JavaUtils.hpp"

namespace ValdiAndroid {

struct InternedStringTranslator {
    using CppType = Valdi::StringBox;
    using JniType = jobject;

    static CppType toCpp(JNIEnv* jniEnv, JniType o) {
        return toInternedString(JavaEnv(), reinterpret_cast<jstring>(o));
    }

    static ::djinni::LocalRef<JniType> fromCpp(JNIEnv* jniEnv, const CppType& c) {
        auto ref = toJavaObject(JavaEnv(), c).releaseObject();
        return ::djinni::LocalRef<JniType>(ref);
    }

    using Boxed = InternedStringTranslator;
};

} // namespace ValdiAndroid
