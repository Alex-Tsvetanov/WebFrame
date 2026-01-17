//
//  JavaField.hpp
//  valdi_core-android
//
//  Created by Simon Corsin on 2/13/23.
//

#pragma once

#include "valdi_core/jni/JavaValue.hpp"

namespace ValdiAndroid {

class JavaField {
public:
    JavaField();
    JavaField(jfieldID fieldId);

    void setFieldId(jfieldID fieldId);
    jfieldID getFieldId() const;

private:
    jfieldID _fieldId = 0;
};

/**
 Implementation of JavaField that uses a delegated getter/setter implementation.
 It allows code to interact with a JVM field without knowing what the type of the
 field is.
 */
class AnyJavaField : public JavaField {
public:
    using FieldGetter = JavaValue (*)(JNIEnv& env, jfieldID fieldID, jobject object);
    using FieldSetter = void (*)(JNIEnv& env, jfieldID fieldID, jobject object, const JavaValue& value);

    struct FieldFuncs {
        FieldGetter getter = nullptr;
        FieldSetter setter = nullptr;

        constexpr FieldFuncs(FieldGetter getter, FieldSetter setter) : getter(getter), setter(setter) {}
    };

    AnyJavaField();
    AnyJavaField(jfieldID fieldId, FieldFuncs* fieldFuncs);

    JavaValue getFieldValue(jobject object) const;
    void setFieldValue(jobject object, const JavaValue& value);

    static FieldFuncs* getFieldFuncsForValueType(JavaValueType valueType, bool isStatic);

    static AnyJavaField fromReflectedField(jobject reflectedField, JavaValueType valueType, bool isStatic);

private:
    FieldFuncs* _fieldFuncs = nullptr;
};

} // namespace ValdiAndroid
