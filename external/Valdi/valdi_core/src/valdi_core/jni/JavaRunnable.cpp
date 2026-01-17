//
//  JavaRunnable.cpp
//  valdi_core-android
//
//  Created by Simon Corsin on 4/19/22.
//

#include "valdi_core/jni/JavaRunnable.hpp"
#include "valdi_core/jni/JavaCache.hpp"
#include "valdi_core/jni/JavaUtils.hpp"

namespace ValdiAndroid {

JavaRunnable::JavaRunnable(JavaEnv env, jobject jobject) : GlobalRefJavaObject(env, jobject, "Runnable", false) {}
JavaRunnable::~JavaRunnable() = default;

void JavaRunnable::operator()() {
    auto& jniCache = JavaEnv::getCache();
    jniCache.getValdiFunctionNativePerformRunnableFromNativeMethod().call(
        jniCache.getValdiFunctionNativeClass().getClass(), RunnableType(getEnv(), get()));
}

} // namespace ValdiAndroid
