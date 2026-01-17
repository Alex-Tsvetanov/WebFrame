//
//  JavaField.cpp
//  valdi_core-android
//
//  Created by Simon Corsin on 2/13/23.
//

#include "valdi_core/jni/JavaField.hpp"
#include "valdi_core/jni/JavaEnv.hpp"
#include "valdi_core/jni/JavaUtils.hpp"

namespace ValdiAndroid {

JavaField::JavaField() = default;
JavaField::JavaField(jfieldID fieldId) : _fieldId(fieldId) {}

jfieldID JavaField::getFieldId() const {
    return _fieldId;
}

void JavaField::setFieldId(jfieldID fieldId) {
    _fieldId = fieldId;
}

AnyJavaField::AnyJavaField() = default;
AnyJavaField::AnyJavaField(jfieldID fieldId, FieldFuncs* fieldFuncs) : JavaField(fieldId), _fieldFuncs(fieldFuncs) {}

JavaValue AnyJavaField::getFieldValue(jobject object) const {
    return _fieldFuncs->getter(*JavaEnv::getUnsafeEnv(), getFieldId(), object);
}

void AnyJavaField::setFieldValue(jobject object, const JavaValue& value) {
    _fieldFuncs->setter(*JavaEnv::getUnsafeEnv(), getFieldId(), object, value);
}

template<typename T>
inline static T asJavaValueInput(T input) {
    return input;
}

inline static djinni::LocalRef<jobject> asJavaValueInput(jobject input) {
    return djinni::LocalRef<jobject>(input);
}

#define VALDI_JAVA_FIELD_FUNCS(__var__, __type__)                                                                      \
    static auto __var__ = AnyJavaField::FieldFuncs(                                                                    \
        [](JNIEnv& env, jfieldID fieldID, jobject object) -> JavaValue {                                               \
            return JavaValue::make##__type__(asJavaValueInput(env.Get##__type__##Field(object, fieldID)));             \
        },                                                                                                             \
        [](JNIEnv& env, jfieldID fieldID, jobject object, const JavaValue& value) -> void {                            \
            env.Set##__type__##Field(object, fieldID, value.get##__type__());                                          \
        });

#define VALDI_JAVA_STATIC_FIELD_FUNCS(__var__, __type__)                                                               \
    static auto __var__ = AnyJavaField::FieldFuncs(                                                                    \
        [](JNIEnv& env, jfieldID fieldID, jobject object) -> JavaValue {                                               \
            return JavaValue::make##__type__(                                                                          \
                asJavaValueInput(env.GetStatic##__type__##Field(reinterpret_cast<jclass>(object), fieldID)));          \
        },                                                                                                             \
        [](JNIEnv& env, jfieldID fieldID, jobject object, const JavaValue& value) -> void {                            \
            env.SetStatic##__type__##Field(reinterpret_cast<jclass>(object), fieldID, value.get##__type__());          \
        });

#define VALDI_RETURN_JAVA_FIELD_FUNCS(isStatic, __type__)                                                              \
    if (isStatic) {                                                                                                    \
        VALDI_JAVA_STATIC_FIELD_FUNCS(kFuncs, __type__);                                                               \
        return &kFuncs;                                                                                                \
    } else {                                                                                                           \
        VALDI_JAVA_FIELD_FUNCS(kFuncs, __type__);                                                                      \
        return &kFuncs;                                                                                                \
    }

AnyJavaField::FieldFuncs* AnyJavaField::getFieldFuncsForValueType(JavaValueType valueType, bool isStatic) {
    switch (valueType) {
        case JavaValueType::Boolean: {
            VALDI_RETURN_JAVA_FIELD_FUNCS(isStatic, Boolean);
        }
        case JavaValueType::Byte: {
            VALDI_RETURN_JAVA_FIELD_FUNCS(isStatic, Byte);
        }
        case JavaValueType::Char: {
            VALDI_RETURN_JAVA_FIELD_FUNCS(isStatic, Char);
        }
        case JavaValueType::Short: {
            VALDI_RETURN_JAVA_FIELD_FUNCS(isStatic, Short);
        }
        case JavaValueType::Int: {
            VALDI_RETURN_JAVA_FIELD_FUNCS(isStatic, Int);
        }
        case JavaValueType::Long: {
            VALDI_RETURN_JAVA_FIELD_FUNCS(isStatic, Long);
        }
        case JavaValueType::Float: {
            VALDI_RETURN_JAVA_FIELD_FUNCS(isStatic, Float);
        }
        case JavaValueType::Double: {
            VALDI_RETURN_JAVA_FIELD_FUNCS(isStatic, Double);
        }
        case JavaValueType::Object: {
            VALDI_RETURN_JAVA_FIELD_FUNCS(isStatic, Object);
        }
        case JavaValueType::Pointer: {
            std::abort();
        }
    }
}

AnyJavaField AnyJavaField::fromReflectedField(jobject reflectedField, JavaValueType valueType, bool isStatic) {
    auto* fieldId = JavaEnv::accessEnvRet([&](JNIEnv& env) { return env.FromReflectedField(reflectedField); });
    auto* fieldFuncs = getFieldFuncsForValueType(valueType, isStatic);

    return AnyJavaField(fieldId, fieldFuncs);
}

} // namespace ValdiAndroid
