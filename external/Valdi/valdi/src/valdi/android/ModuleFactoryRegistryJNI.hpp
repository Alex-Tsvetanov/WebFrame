#include "valdi_core/NativeModuleFactoriesProvider.hpp"
#include "valdi_core/cpp/JavaScript/ModuleFactoryRegistry.hpp"
#include "valdi_core/jni/JNIMethodUtils.hpp"
#include "valdi_core/jni/JavaUtils.hpp"
#include <fbjni/fbjni.h>

namespace fbjni = facebook::jni;

namespace ValdiAndroid {

class ModuleFactoryRegisterJNI : public fbjni::JavaClass<ModuleFactoryRegisterJNI> {
public:
    static constexpr auto kJavaDescriptor = "Lcom/snap/valdi/modules/ModuleFactoryRegistry;";

    // NOLINTNEXTLINE
    static void nativeRegister(fbjni::alias_ref<fbjni::JClass> /* clazz */, jobject moduleFactoriesProvider) {
        auto* env = fbjni::Environment::current();
        auto cppInstance =
            djinni_generated_client::valdi_core::NativeModuleFactoriesProvider::toCpp(env, moduleFactoriesProvider);
        Valdi::ModuleFactoryRegistry::sharedInstance()->registerModuleFactoriesProvider(cppInstance);
    }

    static void registerNatives() {
        javaClassStatic()->registerNatives({
            makeNativeMethod("nativeRegister", ModuleFactoryRegisterJNI::nativeRegister),
        });
    }
};

} // namespace ValdiAndroid
