//
//  JavaMethod.cpp
//  ValdiAndroidNative
//
//  Created by Simon Corsin on 6/11/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

#include "valdi_core/jni/JavaMethod.hpp"
#include <sstream>

namespace ValdiAndroid {

template<>
VoidType callJniMethod(JNIEnv& env, jmethodID methodId, jobject object, jvalue* args) {
    env.CallVoidMethodA(object, methodId, args);
    return VoidType();
}

template<>
jobject callJniMethod(JNIEnv& env, jmethodID methodId, jobject object, jvalue* args) {
    return env.CallObjectMethodA(object, methodId, args);
}

template<>
double callJniMethod(JNIEnv& env, jmethodID methodId, jobject object, jvalue* args) {
    return env.CallDoubleMethodA(object, methodId, args);
}

template<>
bool callJniMethod(JNIEnv& env, jmethodID methodId, jobject object, jvalue* args) {
    return static_cast<bool>(env.CallBooleanMethodA(object, methodId, args));
}

template<>
float callJniMethod(JNIEnv& env, jmethodID methodId, jobject object, jvalue* args) {
    return env.CallFloatMethodA(object, methodId, args);
}

template<>
int8_t callJniMethod(JNIEnv& env, jmethodID methodId, jobject object, jvalue* args) {
    return env.CallByteMethodA(object, methodId, args);
}

template<>
uint16_t callJniMethod(JNIEnv& env, jmethodID methodId, jobject object, jvalue* args) {
    return env.CallCharMethodA(object, methodId, args);
}

template<>
int16_t callJniMethod(JNIEnv& env, jmethodID methodId, jobject object, jvalue* args) {
    return env.CallShortMethodA(object, methodId, args);
}

template<>
int32_t callJniMethod(JNIEnv& env, jmethodID methodId, jobject object, jvalue* args) {
    return env.CallIntMethodA(object, methodId, args);
}

template<>
int64_t callJniMethod(JNIEnv& env, jmethodID methodId, jobject object, jvalue* args) {
    return env.CallLongMethodA(object, methodId, args);
}

template<>
djinni::LocalRef<jobject> callJniMethod(JNIEnv& env, jmethodID methodId, jobject object, jvalue* args) {
    auto* out = env.CallObjectMethodA(object, methodId, args);

    return djinni::LocalRef<jobject>(out);
}

template<>
VoidType callJniStaticMethod(JNIEnv& env, jmethodID methodId, jclass cls, jvalue* args) {
    env.CallStaticVoidMethodA(cls, methodId, args);
    return VoidType();
}

template<>
jobject callJniStaticMethod(JNIEnv& env, jmethodID methodId, jclass cls, jvalue* args) {
    return env.CallStaticObjectMethodA(cls, methodId, args);
}

template<>
double callJniStaticMethod(JNIEnv& env, jmethodID methodId, jclass cls, jvalue* args) {
    return env.CallStaticDoubleMethodA(cls, methodId, args);
}

template<>
bool callJniStaticMethod(JNIEnv& env, jmethodID methodId, jclass cls, jvalue* args) {
    return static_cast<bool>(env.CallStaticBooleanMethodA(cls, methodId, args));
}

template<>
float callJniStaticMethod(JNIEnv& env, jmethodID methodId, jclass cls, jvalue* args) {
    return env.CallStaticFloatMethodA(cls, methodId, args);
}

template<>
int8_t callJniStaticMethod(JNIEnv& env, jmethodID methodId, jclass cls, jvalue* args) {
    return env.CallStaticByteMethodA(cls, methodId, args);
}

template<>
uint16_t callJniStaticMethod(JNIEnv& env, jmethodID methodId, jclass cls, jvalue* args) {
    return env.CallStaticCharMethodA(cls, methodId, args);
}

template<>
int16_t callJniStaticMethod(JNIEnv& env, jmethodID methodId, jclass cls, jvalue* args) {
    return env.CallStaticShortMethodA(cls, methodId, args);
}

template<>
int32_t callJniStaticMethod(JNIEnv& env, jmethodID methodId, jclass cls, jvalue* args) {
    return env.CallStaticIntMethodA(cls, methodId, args);
}

template<>
int64_t callJniStaticMethod(JNIEnv& env, jmethodID methodId, jclass cls, jvalue* args) {
    return env.CallStaticLongMethodA(cls, methodId, args);
}

template<>
djinni::LocalRef<jobject> callJniStaticMethod(JNIEnv& env, jmethodID methodId, jclass cls, jvalue* args) {
    auto* out = env.CallStaticObjectMethodA(cls, methodId, args);

    return djinni::LocalRef<jobject>(out);
}

JavaMethodBase::JavaMethodBase(JavaEnv /*env*/, jmethodID methodID) : _methodId(methodID) {}
JavaMethodBase::JavaMethodBase() = default;

jmethodID JavaMethodBase::getMethodId() const {
    return _methodId;
}

void JavaMethodBase::setMethodId(jmethodID methodId) {
    _methodId = methodId;
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
JavaEnv JavaMethodBase::getEnv() const {
    return JavaEnv();
}

AnyJavaMethod::AnyJavaMethod() = default;

AnyJavaMethod::AnyJavaMethod(JavaEnv env, jmethodID methodID, Invoker invoker)
    : JavaMethodBase(env, methodID), _invoker(invoker) {}

JavaValue AnyJavaMethod::call(jobject object, size_t parametersSize, const JavaValue* parameters) const {
    jvalue outParameters[parametersSize];
    for (size_t i = 0; i < parametersSize; i++) {
        outParameters[i] = parameters[i].getValue();
    }

    return JavaEnv::accessEnvRet(
        [&](JNIEnv& env) -> auto { return _invoker(env, getMethodId(), object, &outParameters[0]); });
}

template<typename T>
static AnyJavaMethod::Invoker makeMethodInvoker(bool isStatic) {
    if (isStatic) {
        return [](JNIEnv& env, jmethodID methodId, jobject obj, jvalue* args) -> JavaValue {
            return toJavaValue(JavaEnv(), callJniStaticMethod<T>(env, methodId, reinterpret_cast<jclass>(obj), args));
        };
    } else {
        return [](JNIEnv& env, jmethodID methodId, jobject obj, jvalue* args) -> JavaValue {
            return toJavaValue(JavaEnv(), callJniMethod<T>(env, methodId, obj, args));
        };
    }
}

AnyJavaMethod::Invoker AnyJavaMethod::getInvokerForMethodType(bool isStatic, std::optional<JavaValueType> returnType) {
    if (!returnType) {
        return makeMethodInvoker<VoidType>(isStatic);
    }

    switch (returnType.value()) {
        case JavaValueType::Boolean:
            return makeMethodInvoker<bool>(isStatic);
        case JavaValueType::Byte:
            return makeMethodInvoker<int8_t>(isStatic);
        case JavaValueType::Char:
            return makeMethodInvoker<uint16_t>(isStatic);
        case JavaValueType::Short:
            return makeMethodInvoker<int16_t>(isStatic);
        case JavaValueType::Int:
            return makeMethodInvoker<int32_t>(isStatic);
        case JavaValueType::Long:
            return makeMethodInvoker<int64_t>(isStatic);
        case JavaValueType::Float:
            return makeMethodInvoker<float>(isStatic);
        case JavaValueType::Double:
            return makeMethodInvoker<double>(isStatic);
        case JavaValueType::Object:
            return makeMethodInvoker<djinni::LocalRef<jobject>>(isStatic);
        case JavaValueType::Pointer:
            std::abort();
    }
}

djinni::LocalRef<jobject> AnyJavaMethod::toReflectedMethod(jclass cls, bool isStatic) const {
    return JavaEnv::accessEnvRetRef(
        [&](JNIEnv& env) { return env.ToReflectedMethod(cls, getMethodId(), static_cast<jboolean>(isStatic)); });
}

AnyJavaMethod AnyJavaMethod::fromReflectedMethod(jobject reflectedMethod,
                                                 bool isStatic,
                                                 std::optional<JavaValueType> returnType) {
    auto invoker = getInvokerForMethodType(isStatic, returnType);

    auto* methodId = JavaEnv::accessEnvRet([&](JNIEnv& env) { return env.FromReflectedMethod(reflectedMethod); });

    return AnyJavaMethod(JavaEnv(), methodId, invoker);
}

} // namespace ValdiAndroid
