#include "valdi_core/jni/JNIMethodUtils.hpp"
#include "valdi_core/jni/JavaUtils.hpp"
#include <fbjni/fbjni.h>

namespace fbjni = facebook::jni;

namespace ValdiAndroid {

class InternedStringCPP : public fbjni::JavaClass<InternedStringCPP> {
public:
    static constexpr auto kJavaDescriptor = "Lcom/snap/valdi/utils/InternedStringCPP;";

    //  NOLINTNEXTLINE
    static jlong nativeCreate(fbjni::alias_ref<fbjni::JClass> /* clazz */, fbjni::alias_ref<fbjni::JString> str) {
        return ValdiAndroid::createInternedString(ValdiAndroid::JavaEnv(), str.get());
    }

    //  NOLINTNEXTLINE
    static void nativeRelease(fbjni::alias_ref<fbjni::JClass> /* clazz */, jlong ptr) {
        ValdiAndroid::destroyInternedString(ptr);
    }

    //  NOLINTNEXTLINE
    static void nativeRetain(fbjni::alias_ref<fbjni::JClass> /* clazz */, jlong ptr) {
        auto internedString = ValdiAndroid::unwrapInternedString(ptr);
        Valdi::unsafeRetain(internedString.getInternedString());
    }

    //  NOLINTNEXTLINE
    static jstring nativeToString(fbjni::alias_ref<fbjni::JClass> /* clazz */, jlong ptr) {
        auto internedString = ValdiAndroid::unwrapInternedString(ptr);
        return reinterpret_cast<jstring>(
            ValdiAndroid::toJavaObject(ValdiAndroid::JavaEnv(), internedString).releaseObject());
    }

    static void registerNatives() {
        javaClassStatic()->registerNatives({
            makeNativeMethod("nativeCreate", InternedStringCPP::nativeCreate),
            makeNativeMethod("nativeRelease", InternedStringCPP::nativeRelease),
            makeNativeMethod("nativeRetain", InternedStringCPP::nativeRetain),
            makeNativeMethod("nativeToString", InternedStringCPP::nativeToString),
        });
    }
};

} // namespace ValdiAndroid
