#include "valdi/android/AndroidValueMarshallerRegistry.hpp"
#include "valdi_core/jni/JNIMethodUtils.hpp"
#include "valdi_core/jni/JavaUtils.hpp"
#include <fbjni/fbjni.h>

namespace fbjni = facebook::jni;

namespace ValdiAndroid {

class ValueMarshallerRegistryCppJNI : public fbjni::JavaClass<ValueMarshallerRegistryCppJNI> {
public:
    static constexpr auto kJavaDescriptor = "Lcom/snap/valdi/schema/ValdiValueMarshallerRegistryCpp;";

    // NOLINTNEXTLINE
    static jlong nativeCreate(fbjni::alias_ref<fbjni::JClass> /* clazz */) {
        auto registry = Valdi::makeShared<AndroidValueMarshallerRegistry>();
        return ValdiAndroid::bridgeRetain(registry.get());
    }

    // NOLINTNEXTLINE
    static void nativeDestroy(fbjni::alias_ref<fbjni::JClass> /* clazz */, jlong ptr) {
        ValdiAndroid::bridgeRelease(ptr);
    }

    // NOLINTNEXTLINE
    static jint nativeMarshallObject(fbjni::alias_ref<fbjni::JClass> /* clazz */,
                                     jlong ptr,
                                     jstring className,
                                     jlong marshallerHandle,
                                     jobject object) {
        auto registry = bridge<AndroidValueMarshallerRegistry>(ptr);
        auto classNameCpp = toInternedString(JavaEnv(), className);
        auto* marshaller = unwrapMarshaller(JavaEnv::getUnsafeEnv(), marshallerHandle);

        auto result = registry->marshallObject(classNameCpp, JavaValue::unsafeMakeObject(object));
        if (!result) {
            throwJavaValdiException(JavaEnv::getUnsafeEnv(), result.error());
            return static_cast<jint>(0);
        }

        return static_cast<jint>(marshaller->push(result.moveValue()));
    }

    // NOLINTNEXTLINE
    static jobject nativeMarshallObjectAsMap(fbjni::alias_ref<fbjni::JClass> /* clazz */,
                                             jlong ptr,
                                             jstring className,
                                             jobject object) {
        auto registry = bridge<AndroidValueMarshallerRegistry>(ptr);
        auto classNameCpp = toInternedString(JavaEnv(), className);

        auto result = registry->marshallObject(classNameCpp, JavaValue::unsafeMakeObject(object));
        if (!result) {
            throwJavaValdiException(JavaEnv::getUnsafeEnv(), result.error());
            return nullptr;
        }

        return toJavaObject(JavaEnv(), result.value()).releaseObject();
    }

    // NOLINTNEXTLINE
    static jobject nativeUnmarshallObject(fbjni::alias_ref<fbjni::JClass> /* clazz */,
                                          jlong ptr,
                                          jstring className,
                                          jlong marshallerHandle,
                                          jint objectIndex) {
        auto registry = bridge<AndroidValueMarshallerRegistry>(ptr);
        auto classNameCpp = toInternedString(JavaEnv(), className);
        auto* marshaller = unwrapMarshaller(JavaEnv::getUnsafeEnv(), marshallerHandle);

        auto result =
            registry->unmarshallObject(classNameCpp, marshaller->getOrUndefined(static_cast<int>(objectIndex)));
        if (!result) {
            throwJavaValdiException(JavaEnv::getUnsafeEnv(), result.error());
            return nullptr;
        }

        return result.value().releaseObject();
    }

    // NOLINTNEXTLINE
    static void nativeSetActiveSchema(fbjni::alias_ref<fbjni::JClass> /* clazz */,
                                      jlong ptr,
                                      jstring className,
                                      jlong marshallerHandle) {
        auto registry = bridge<AndroidValueMarshallerRegistry>(ptr);
        auto classNameCpp = toInternedString(JavaEnv(), className);
        auto* marshaller = unwrapMarshaller(JavaEnv::getUnsafeEnv(), marshallerHandle);

        auto result = registry->setActiveSchemaInMarshaller(classNameCpp, *marshaller);
        if (!result) {
            throwJavaValdiException(JavaEnv::getUnsafeEnv(), result.error());
        }
    }

    // NOLINTNEXTLINE
    static jobject nativeGetEnumValue(fbjni::alias_ref<fbjni::JClass> /* clazz */,
                                      jlong ptr,
                                      jstring className,
                                      jobject enumValue) {
        auto registry = bridge<AndroidValueMarshallerRegistry>(ptr);
        auto classNameCpp = toInternedString(JavaEnv(), className);

        auto result = registry->getEnumValue(classNameCpp, JavaValue::unsafeMakeObject(enumValue));
        if (!result) {
            throwJavaValdiException(JavaEnv::getUnsafeEnv(), result.error());
        }

        return toJavaObject(JavaEnv(), result.value()).releaseObject();
    }

    static void registerNatives() {
        javaClassStatic()->registerNatives({
            makeNativeMethod("nativeCreate", ValueMarshallerRegistryCppJNI::nativeCreate),
            makeNativeMethod("nativeDestroy", ValueMarshallerRegistryCppJNI::nativeDestroy),
            makeNativeMethod("nativeMarshallObject", ValueMarshallerRegistryCppJNI::nativeMarshallObject),
            makeNativeMethod("nativeMarshallObjectAsMap", ValueMarshallerRegistryCppJNI::nativeMarshallObjectAsMap),
            makeNativeMethod("nativeUnmarshallObject", ValueMarshallerRegistryCppJNI::nativeUnmarshallObject),
            makeNativeMethod("nativeSetActiveSchema", ValueMarshallerRegistryCppJNI::nativeSetActiveSchema),
            makeNativeMethod("nativeGetEnumValue", ValueMarshallerRegistryCppJNI::nativeGetEnumValue),
        });
    }
};

} // namespace ValdiAndroid
