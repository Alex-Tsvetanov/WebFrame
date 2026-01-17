//
//  JavaMethod.hpp
//  ValdiAndroidNative
//
//  Created by Simon Corsin on 6/11/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

#pragma once

#include "valdi_core/jni/JNIConstants.hpp"
#include "valdi_core/jni/JavaEnv.hpp"
#include "valdi_core/jni/JavaUtils.hpp"
#include "valdi_core/jni/JavaValue.hpp"
#include <djinni/jni/djinni_support.hpp>
#include <jni.h>
#include <string>

namespace ValdiAndroid {

template<typename T>
T callJniMethod(JNIEnv& env, jmethodID methodId, jobject object, jvalue* args) {
    auto* out = env.CallObjectMethodA(object, methodId, args);

    return T(JavaEnv(), ::djinni::LocalRef<jobject>(out));
}

template<>
VoidType callJniMethod(JNIEnv& env, jmethodID methodId, jobject object, jvalue* args);

template<>
::djinni::LocalRef<jobject> callJniMethod(JNIEnv& env, jmethodID methodId, jobject object, jvalue* args);

template<>
double callJniMethod(JNIEnv& env, jmethodID methodId, jobject object, jvalue* args);

template<>
bool callJniMethod(JNIEnv& env, jmethodID methodId, jobject object, jvalue* args);

template<>
float callJniMethod(JNIEnv& env, jmethodID methodId, jobject object, jvalue* args);

template<>
int8_t callJniMethod(JNIEnv& env, jmethodID methodId, jobject object, jvalue* args);

template<>
uint16_t callJniMethod(JNIEnv& env, jmethodID methodId, jobject object, jvalue* args);

template<>
int16_t callJniMethod(JNIEnv& env, jmethodID methodId, jobject object, jvalue* args);

template<>
int32_t callJniMethod(JNIEnv& env, jmethodID methodId, jobject object, jvalue* args);

template<>
int64_t callJniMethod(JNIEnv& env, jmethodID methodId, jobject object, jvalue* args);

template<>
djinni::LocalRef<jobject> callJniMethod(JNIEnv& env, jmethodID methodId, jobject object, jvalue* args);

template<typename T>
T callJniStaticMethod(JNIEnv& env, jmethodID methodId, jclass cls, jvalue* args) {
    auto* out = env.CallStaticObjectMethodA(cls, methodId, args);

    return T(JavaEnv(), ::djinni::LocalRef<jobject>(out));
}

template<>
VoidType callJniStaticMethod(JNIEnv& env, jmethodID methodId, jclass cls, jvalue* args);

template<>
::djinni::LocalRef<jobject> callJniStaticMethod(JNIEnv& env, jmethodID methodId, jclass cls, jvalue* args);

template<>
double callJniStaticMethod(JNIEnv& env, jmethodID methodId, jclass cls, jvalue* args);

template<>
bool callJniStaticMethod(JNIEnv& env, jmethodID methodId, jclass cls, jvalue* args);

template<>
float callJniStaticMethod(JNIEnv& env, jmethodID methodId, jclass cls, jvalue* args);

template<>
int8_t callJniStaticMethod(JNIEnv& env, jmethodID methodId, jclass cls, jvalue* args);

template<>
uint16_t callJniStaticMethod(JNIEnv& env, jmethodID methodId, jclass cls, jvalue* args);

template<>
int16_t callJniStaticMethod(JNIEnv& env, jmethodID methodId, jclass cls, jvalue* args);

template<>
int32_t callJniStaticMethod(JNIEnv& env, jmethodID methodId, jclass cls, jvalue* args);

template<>
int64_t callJniStaticMethod(JNIEnv& env, jmethodID methodId, jclass cls, jvalue* args);

template<>
djinni::LocalRef<jobject> callJniStaticMethod(JNIEnv& env, jmethodID methodId, jclass cls, jvalue* args);

class JavaMethodBase {
public:
    JavaMethodBase(JavaEnv env, jmethodID methodID);
    JavaMethodBase();

    jmethodID getMethodId() const;

    JavaEnv getEnv() const;

    void setMethodId(jmethodID methodId);

private:
    jmethodID _methodId = nullptr;
};

/**
 Implementation of JavaMethodBase that does not leverage the C++ type system
 to resolve the parameters and return value. This implementation is suitable
 for use in a purely reflective environment, where types are resolved from
 external sources and not from existing C++ types.
 */
class AnyJavaMethod : public JavaMethodBase {
public:
    using Invoker = JavaValue (*)(JNIEnv& env, jmethodID methodId, jobject obj, jvalue* args);

    AnyJavaMethod();
    AnyJavaMethod(JavaEnv env, jmethodID methodID, Invoker invoker);

    JavaValue call(jobject object, size_t parametersSize, const JavaValue* parameters) const;

    djinni::LocalRef<jobject> toReflectedMethod(jclass cls, bool isStatic) const;

    static AnyJavaMethod fromReflectedMethod(jobject reflectedMethod,
                                             bool isStatic,
                                             std::optional<JavaValueType> returnType);
    static Invoker getInvokerForMethodType(bool isStatic, std::optional<JavaValueType> returnType);

private:
    Invoker _invoker = nullptr;
};

template<class Ret, class... T>
class JavaMethod : public JavaMethodBase {
public:
    JavaMethod(JavaEnv env, jmethodID methodID) : JavaMethodBase(env, methodID) {}
    JavaMethod() = default;

    Ret call(const JavaObject& object, const T&... args) const {
        return call(object.getUnsafeObject(), args...);
    }

    Ret call(jobject object, const T&... args) const {
        const int ksize = sizeof...(args);
        JavaValue parameters[ksize] = {toJavaValue(getEnv(), args)...};

        jvalue outParameters[ksize];
        for (int i = 0; i < ksize; i++) {
            outParameters[i] = parameters[i].getValue();
        }

        return JavaEnv::accessEnvRet(
            [&](JNIEnv& env) -> auto { return callJniMethod<Ret>(env, getMethodId(), object, &outParameters[0]); });
    }
};

template<class Ret, class... T>
class JavaStaticMethod : public JavaMethodBase {
public:
    JavaStaticMethod(JavaEnv env, jmethodID methodID) : JavaMethodBase(env, methodID) {}
    JavaStaticMethod() = default;

    Ret call(jclass cls, const T&... args) const {
        const int ksize = sizeof...(args);
        JavaValue parameters[ksize] = {toJavaValue(getEnv(), args)...};

        jvalue outParameters[ksize];
        for (int i = 0; i < ksize; i++) {
            outParameters[i] = parameters[i].getValue();
        }

        return JavaEnv::accessEnvRet(
            [&](JNIEnv& env) -> auto { return callJniStaticMethod<Ret>(env, getMethodId(), cls, &outParameters[0]); });
    }
};

} // namespace ValdiAndroid
