//
//  JavaClass.cpp
//  ValdiAndroidNative
//
//  Created by Simon Corsin on 6/11/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

#include "valdi_core/jni/JavaClass.hpp"
#include "djinni/jni/djinni_support.hpp"
#include "valdi_core/cpp/Schema/ValueSchema.hpp"
#include "valdi_core/jni/JavaUtils.hpp"

namespace ValdiAndroid {

#ifdef VALDI_DEBUG_JNI_CALLS

Valdi::StringBox resolveClassName(JavaEnv env, jclass cls) {
    static jmethodID classNameMethodId = ::djinni::jniGetMethodID(cls, "getName", "()Ljava/lang/String;");
    auto value = ::djinni::LocalRef<jobject>(env.getUnsafeEnv()->CallObjectMethod(cls, classNameMethodId));

    return ValdiAndroid::toInternedString(env, reinterpret_cast<jstring>(value.get()));
}

#endif

JavaClass::JavaClass(JavaEnv /*env*/, jclass cls) : _class(JavaEnv::newGlobalRef(cls)) {
#ifdef VALDI_DEBUG_JNI_CALLS
    _className = resolveClassName(env, getClass());
#endif
}

JavaClass::JavaClass() = default;
JavaClass::JavaClass(JavaClass&& other) noexcept : _class(std::move(other._class)) {}

JavaClass& JavaClass::operator=(JavaClass&& other) noexcept {
    _class = std::move(other._class);
    return *this;
}

JavaClass JavaClass::resolveOrAbort(JavaEnv env, const char* className) {
    auto jClass = JavaEnv::accessEnvRetRef([&](JNIEnv& jniEnv) -> jclass { return jniEnv.FindClass(className); });

    return JavaClass(env, jClass);
}

JavaClass JavaClass::fromReflectedClass(JavaEnv env, jobject reflectedClass) {
    auto* jniEnv = ValdiAndroid::JavaEnv::getUnsafeEnv();
    auto jClass = djinni::LocalRef<jclass>(jniEnv->GetObjectClass(reflectedClass));
    return JavaClass(env, jClass);
}

Valdi::Result<JavaClass> JavaClass::resolve(JavaEnv env, const char* className) {
    auto* jniEnv = ValdiAndroid::JavaEnv::getUnsafeEnv();
    auto jClass = djinni::LocalRef<jclass>(jniEnv->FindClass(className));

    if (jniEnv->ExceptionCheck() != 0L) {
        jniEnv->ExceptionClear();
    }

    if (jClass == nullptr) {
        return Valdi::Error("Did not find class");
    }

    return JavaClass(env, jClass);
}

::djinni::LocalRef<jobject> JavaClass::newObjectUntyped(const JavaMethodBase& constructor,
                                                        const JavaValue* parameters,
                                                        size_t parametersSize) const {
    jvalue outParameters[parametersSize];
    for (size_t i = 0; i < parametersSize; i++) {
        outParameters[i] = parameters[i].getValue(); // NOLINT
    }

    return JavaEnv::accessEnvRetRef(
        [&](JNIEnv& env) -> auto { return env.NewObjectA(getClass(), constructor.getMethodId(), &outParameters[0]); });
}

static AnyJavaField::FieldFuncs* resolveFieldsFunc(const Valdi::ValueSchema& schema, bool isStatic) {
    auto valueType = schemaToJavaValueType(schema);
    SC_ASSERT(valueType.has_value());
    return AnyJavaField::getFieldFuncsForValueType(valueType.value(), isStatic);
}

AnyJavaMethod JavaClass::getMethod(const char* name,
                                   const Valdi::ValueFunctionSchema& functionSchema,
                                   bool treatArrayAsList) const {
    return doGetMethod(name,
                       false,
                       functionSchema.getReturnValue(),
                       functionSchema.getParameters(),
                       functionSchema.getParametersSize(),
                       treatArrayAsList);
}

AnyJavaMethod JavaClass::getMethod(const char* name,
                                   const Valdi::ValueSchema& returnType,
                                   const Valdi::ValueSchema* parameters,
                                   size_t parametersSize,
                                   bool treatArrayAsList) const {
    return doGetMethod(name, false, returnType, parameters, parametersSize, treatArrayAsList);
}

AnyJavaMethod JavaClass::getStaticMethod(const char* name,
                                         const Valdi::ValueFunctionSchema& functionSchema,
                                         bool treatArrayAsList) const {
    return doGetMethod(name,
                       true,
                       functionSchema.getReturnValue(),
                       functionSchema.getParameters(),
                       functionSchema.getParametersSize(),
                       treatArrayAsList);
}

AnyJavaMethod JavaClass::getStaticMethod(const char* name,
                                         const Valdi::ValueSchema& returnType,
                                         const Valdi::ValueSchema* parameters,
                                         size_t parametersSize,
                                         bool treatArrayAsList) const {
    return doGetMethod(name, true, returnType, parameters, parametersSize, treatArrayAsList);
}

AnyJavaMethod JavaClass::doGetMethod(const char* name,
                                     bool isStatic,
                                     const Valdi::ValueSchema& returnType,
                                     const Valdi::ValueSchema* parameters,
                                     size_t parametersSize,
                                     bool treatArrayAsList) const {
    auto returnTypeEncoding = getJavaTypeSignatureFromValueSchema(returnType, treatArrayAsList);
    auto functionTypeEncoding = getJavaTypeEncoding(parameters, parametersSize, returnTypeEncoding, treatArrayAsList);

    auto* methodId = resolveMethodId(name, isStatic, functionTypeEncoding);
    auto methodInvoker = AnyJavaMethod::getInvokerForMethodType(isStatic, schemaToJavaValueType(returnType));

    return AnyJavaMethod(JavaEnv(), methodId, methodInvoker);
}

JavaMethodBase JavaClass::getConstructor(const Valdi::ValueSchema* parameters,
                                         size_t parametersSize,
                                         bool treatArrayAsList) const {
    auto functionTypeEncoding =
        getJavaTypeEncoding(parameters, parametersSize, kJniSigConstructorRetType, treatArrayAsList);

    auto* methodId = resolveMethodId(kJniSigConstructor, false, functionTypeEncoding);

    return JavaMethodBase(JavaEnv(), methodId);
}

std::string JavaClass::getJavaTypeEncoding(const Valdi::ValueSchema* parameters,
                                           size_t parametersSize,
                                           std::string_view returnTypeEncoding,
                                           bool treatArrayAsList) {
    std::ostringstream s;
    s << "(";

    for (size_t i = 0; i < parametersSize; i++) {
        s << getJavaTypeSignatureFromValueSchema(parameters[i], treatArrayAsList);
    }

    s << ")";
    s << returnTypeEncoding;

    return s.str();
}

jmethodID JavaClass::resolveMethodId(const char* name, bool isStatic, const std::string& methodSignature) const {
#ifdef VALDI_DEBUG_JNI_CALLS
    __android_log_print(ANDROID_LOG_INFO,
                        "[valdi_jni]",
                        "Going to resolve static method in %s: %s",
                        _className.getCStr(),
                        methodSignature.c_str());
#endif

    auto* methodID = JavaEnv::accessEnvRet([&](JNIEnv& env) -> jmethodID {
        if (isStatic) {
            return env.GetStaticMethodID(getClass(), name, methodSignature.c_str());
        } else {
            return env.GetMethodID(getClass(), name, methodSignature.c_str());
        }
    });

#ifdef VALDI_DEBUG_JNI_CALLS
    __android_log_print(ANDROID_LOG_INFO,
                        "[valdi_jni]",
                        "Done resolve static method in %s: %s",
                        _className.getCStr(),
                        methodSignature.c_str());
#endif

    return methodID;
}

jfieldID JavaClass::resolveFieldId(const char* name, bool isStatic, std::string_view typeSignature) const {
    return JavaEnv::accessEnvRet([&](JNIEnv& env) -> jfieldID {
        if (isStatic) {
            return env.GetStaticFieldID(getClass(), name, typeSignature.data());
        } else {
            return env.GetFieldID(getClass(), name, typeSignature.data());
        }
    });
}

AnyJavaField JavaClass::getField(const char* name, const Valdi::ValueSchema& schema, bool treatArrayAsList) const {
    return doGetField(name, false, schema, treatArrayAsList);
}

AnyJavaField JavaClass::getStaticField(const char* name,
                                       const Valdi::ValueSchema& schema,
                                       bool treatArrayAsList) const {
    return doGetField(name, true, schema, treatArrayAsList);
}

AnyJavaField JavaClass::doGetField(const char* name,
                                   bool isStatic,
                                   const Valdi::ValueSchema& schema,
                                   bool treatArrayAsList) const {
    auto typeSignature = getJavaTypeSignatureFromValueSchema(schema, treatArrayAsList);
    auto* fieldId = resolveFieldId(name, isStatic, typeSignature);

    return AnyJavaField(fieldId, resolveFieldsFunc(schema, isStatic));
}

JavaClass::~JavaClass() = default;

jclass JavaClass::getClass() const {
    return _class.get();
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
JavaEnv JavaClass::getEnv() const {
    return JavaEnv();
}

} // namespace ValdiAndroid
