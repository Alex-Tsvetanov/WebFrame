#include "utils/platform/JvmOnLoad.hpp"

#include "utils/platform/TargetPlatform.hpp"

#if defined(SC_ANDROID)

namespace snap::utils::platform::jni {

std::vector<JniInitializer>& getInitializersVector() {
    static std::vector<JniInitializer> initializers;
    return initializers;
}
void runOnLoadInitializers(JNIEnv* env) {
    auto& initializers = getInitializersVector();
    for (auto& initializer : initializers) {
        initializer(env);
    }
    initializers.clear();
}

} // namespace snap::utils::platform::jni

#endif
