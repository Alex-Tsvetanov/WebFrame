#pragma once

#include "djinni/jni/djinni_support.hpp"
#include "valdi/runtime/Views/View.hpp"
#include "valdi_core/jni/JavaUtils.hpp"

namespace ValdiAndroid {

struct ViewTranslator {
    using CppType = Valdi::Ref<Valdi::View>;
    using JniType = jobject;

    static ::djinni::LocalRef<JniType> fromCpp(JNIEnv* jniEnv, const CppType& c) {
        auto object = javaObjectFromValdiObject(c).releaseObject();
        return ::djinni::LocalRef<JniType>(object);
    }

    using Boxed = Valdi::Ref<Valdi::View>;
};

} // namespace ValdiAndroid
