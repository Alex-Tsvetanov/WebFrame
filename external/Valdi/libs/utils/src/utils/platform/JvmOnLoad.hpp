#include "utils/platform/TargetPlatform.hpp"

#if defined(SC_ANDROID)

#include <jni.h>
#include <vector>

// Provides a way for you to run your own code during JNI_OnLoad. Typically
// this would be used to register JNI methods.

namespace snap::utils::platform::jni {
void runOnLoadInitializers(JNIEnv* env);
using JniInitializer = void (*)(JNIEnv*);
std::vector<JniInitializer>& getInitializersVector();
} // namespace snap::utils::platform::jni

#define SNAP_JNI_DEFINE_INITIALIZER(initializer)                                                                       \
    static __attribute__((constructor)) void registerOnLoadInitializer(void) {                                         \
        snap::utils::platform::jni::getInitializersVector().push_back(initializer);                                    \
    }

#endif
