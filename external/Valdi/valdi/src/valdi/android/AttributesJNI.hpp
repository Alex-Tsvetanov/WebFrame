#include "valdi_core/jni/JNIMethodUtils.hpp"
#include "valdi_core/jni/JavaUtils.hpp"
#include <fbjni/fbjni.h>

namespace fbjni = facebook::jni;

namespace ValdiAndroid {

class AttributesJNI : public fbjni::JavaClass<AttributesJNI> {
public:
    static constexpr auto kJavaDescriptor = "Lcom/snap/valdi/attributes/ViewLayoutAttributesCpp;";

    static Valdi::Value getValue(jlong ptr, jstring attributeName) {
        auto attributeNameCpp = toInternedString(JavaEnv(), attributeName);
        auto attributes = Valdi::unsafeBridge<Valdi::ValueMap>(reinterpret_cast<void*>(ptr));

        const auto& it = attributes->find(attributeNameCpp);
        if (it == attributes->end()) {
            return Valdi::Value();
        }

        return it->second;
    }

    // NOLINTNEXTLINE
    static jobject nativeGetValue(fbjni::alias_ref<fbjni::JClass> /* clazz */, jlong ptr, jstring attributeName) {
        auto value = getValue(ptr, attributeName);
        return toJavaObject(JavaEnv(), value).releaseObject();
    }

    // NOLINTNEXTLINE
    static jboolean nativeGetBoolValue(fbjni::alias_ref<fbjni::JClass> /* clazz */, jlong ptr, jstring attributeName) {
        auto value = getValue(ptr, attributeName);
        return static_cast<jboolean>(value.toBool());
    }

    // NOLINTNEXTLINE
    static jdouble nativeGetDoubleValue(fbjni::alias_ref<fbjni::JClass> /* clazz */, jlong ptr, jstring attributeName) {
        auto value = getValue(ptr, attributeName);
        return static_cast<jdouble>(value.toDouble());
    }

    static void registerNatives() {
        javaClassStatic()->registerNatives({
            makeNativeMethod("nativeGetValue", AttributesJNI::nativeGetValue),
            makeNativeMethod("nativeGetBoolValue", AttributesJNI::nativeGetBoolValue),
            makeNativeMethod("nativeGetDoubleValue", AttributesJNI::nativeGetDoubleValue),
        });
    }
};

} // namespace ValdiAndroid
