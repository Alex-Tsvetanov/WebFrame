//
//  JNIMethodUtils.cpp
//  valdi-android
//
//  Created by Simon Corsin on 7/24/19.
//

#include "valdi_core/jni/JNIMethodUtils.hpp"
#include "valdi_core/jni/JavaCache.hpp"

namespace ValdiAndroid {

void throwJavaValdiException(JNIEnv* env, const Valdi::Error& error) {
    auto errorString = error.toString();
    throwJavaValdiException(env, errorString.c_str());
}

void throwJavaValdiException(JNIEnv* env, const char* message) {
    env->ThrowNew(ValdiAndroid::JavaEnv::getCache().getValdiExceptionClass().getClass(), message);
}

} // namespace ValdiAndroid
