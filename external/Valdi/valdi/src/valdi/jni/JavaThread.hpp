#include "valdi_core/cpp/Threading/Thread.hpp"
#include "valdi_core/jni/GlobalRefJavaObject.hpp"

namespace ValdiAndroid {
class JavaThread : public GlobalRefJavaObject {
public:
    JavaThread(JavaEnv env, jobject ref);
    ~JavaThread() override;

    static void registerNatives();
};
} // namespace ValdiAndroid
