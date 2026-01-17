//
//  JavaClass.hpp
//  ValdiAndroidNative
//
//  Created by Simon Corsin on 6/11/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

#pragma once

#include "valdi_core/jni/JNIConstants.hpp"
#include "valdi_core/jni/JavaEnv.hpp"
#include "valdi_core/jni/JavaField.hpp"
#include "valdi_core/jni/JavaMethod.hpp"
#if ANDROID
#include <android/log.h>
#endif
#include <djinni/jni/djinni_support.hpp>
#include <jni.h>
#include <sstream>
#include <string>

#include "utils/base/NonCopyable.hpp"
#include "valdi_core/cpp/Utils/Result.hpp"

namespace Valdi {
class ValueFunctionSchema;
class ValueSchema;
} // namespace Valdi

namespace ValdiAndroid {

class JavaEnv;

template<typename Ret, typename... T>
std::string getMethodSignature() {
    std::ostringstream s;
    s << "(";

    const int kcount = sizeof...(T);

    const char* allSignatures[kcount] = {GetJNIType<T>()()...};
    for (auto signature : allSignatures) {
        s << signature;
    }

    s << ")";
    s << GetJNIType<Ret>()();

    return s.str();
}

class JavaClass : public snap::NonCopyable {
public:
    JavaClass();
    JavaClass(JavaClass&& other) noexcept;
    JavaClass(JavaEnv env, jclass cls);
    ~JavaClass();

    JavaEnv getEnv() const;

    JavaClass& operator=(JavaClass&& other) noexcept;

    static Valdi::Result<JavaClass> resolve(JavaEnv env, const char* className);
    static JavaClass resolveOrAbort(JavaEnv env, const char* className);
    static JavaClass fromReflectedClass(JavaEnv env, jobject reflectedClass);

    template<typename Ret, typename... T>
    jmethodID getMethodID(const char* name) {
        auto methodSignature = getMethodSignature<Ret, T...>();
        return resolveMethodId(name, false, methodSignature);
    }

    template<typename Ret, typename... T>
    jmethodID getStaticMethodID(const char* name) {
        auto methodSignature = getMethodSignature<Ret, T...>();
        return resolveMethodId(name, true, methodSignature);
    }

    template<typename Ret, typename... T>
    JavaMethod<Ret, T...> getMethod(const char* name) {
        auto methodID = getMethodID<Ret, T...>(name);

        return JavaMethod<Ret, T...>(getEnv(), methodID);
    }

    template<typename Ret, typename... T>
    JavaStaticMethod<Ret, T...> getStaticMethod(const char* name) {
        auto methodID = getStaticMethodID<Ret, T...>(name);

        return JavaStaticMethod<Ret, T...>(getEnv(), methodID);
    }

    template<typename Ret, typename... T>
    void getStaticMethod(const char* name, JavaStaticMethod<Ret, T...>& method) {
        auto methodID = getStaticMethodID<Ret, T...>(name);
        method.setMethodId(methodID);
    }

    template<typename Ret, typename... T>
    void getMethod(const char* name, JavaMethod<Ret, T...>& method) {
        auto methodID = getMethodID<Ret, T...>(name);
        method.setMethodId(methodID);
    }

    template<typename FieldType>
    jfieldID getFieldId(const char* fieldName) {
        return resolveFieldId(fieldName, false, GetJNIType<FieldType>()());
    }

    template<typename FieldType>
    jfieldID getStaticFieldId(const char* fieldName) {
        return resolveFieldId(fieldName, true, GetJNIType<FieldType>()());
    }

    ::djinni::LocalRef<jobject> newObjectUntyped(const JavaMethodBase& constructor,
                                                 const JavaValue* parameters,
                                                 size_t parametersSize) const;

    template<typename... T>
    ::djinni::LocalRef<jobject> newObject(JavaMethod<ConstructorType, T...>& constructor, T... args) {
        const int ksize = sizeof...(args);
        JavaValue parameters[ksize] = {toJavaValue(getEnv(), args)...};

        return newObjectUntyped(constructor, &parameters[0], ksize);
    }

    AnyJavaMethod getMethod(const char* name,
                            const Valdi::ValueFunctionSchema& functionSchema,
                            bool treatArrayAsList) const;
    AnyJavaMethod getMethod(const char* name,
                            const Valdi::ValueSchema& returnType,
                            const Valdi::ValueSchema* parameters,
                            size_t parametersSize,
                            bool treatArrayAsList) const;

    AnyJavaMethod getStaticMethod(const char* name,
                                  const Valdi::ValueFunctionSchema& functionSchema,
                                  bool treatArrayAsList) const;
    AnyJavaMethod getStaticMethod(const char* name,
                                  const Valdi::ValueSchema& returnType,
                                  const Valdi::ValueSchema* parameters,
                                  size_t parametersSize,
                                  bool treatArrayAsList) const;
    JavaMethodBase getConstructor(const Valdi::ValueSchema* parameters,
                                  size_t parametersSize,
                                  bool treatArrayAsList) const;

    AnyJavaField getField(const char* name, const Valdi::ValueSchema& schema, bool treatArrayAsList) const;
    AnyJavaField getStaticField(const char* name, const Valdi::ValueSchema& schema, bool treatArrayAsList) const;

    jclass getClass() const;

private:
    ::djinni::GlobalRef<jclass> _class;
#ifdef VALDI_DEBUG_JNI_CALLS
    Valdi::StringBox _className;
#endif

    AnyJavaMethod doGetMethod(const char* name,
                              bool isStatic,
                              const Valdi::ValueSchema& returnType,
                              const Valdi::ValueSchema* parameters,
                              size_t parametersSize,
                              bool treatArrayAsList) const;

    jmethodID resolveMethodId(const char* name, bool isStatic, const std::string& methodSignature) const;

    jfieldID resolveFieldId(const char* name, bool isStatic, std::string_view typeSignature) const;
    AnyJavaField doGetField(const char* name,
                            bool isStatic,
                            const Valdi::ValueSchema& schema,
                            bool treatArrayAsList) const;

    static std::string getJavaTypeEncoding(const Valdi::ValueSchema* parameters,
                                           size_t parametersSize,
                                           std::string_view returnTypeEncoding,
                                           bool treatArrayAsList);
};

} // namespace ValdiAndroid
