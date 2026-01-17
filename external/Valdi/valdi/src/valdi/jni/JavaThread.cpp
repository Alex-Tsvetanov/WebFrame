#include "valdi/jni/JavaThread.hpp"
#include "valdi_core/cpp/Utils/ConsoleLogger.hpp"
#include "valdi_core/jni/JNIMethodUtils.hpp"
#include "valdi_core/jni/JavaCache.hpp"
#include "valdi_core/jni/JavaUtils.hpp"
#include "valdi_core/jni/ScopedJNIExceptionHandler.hpp"

namespace ValdiAndroid {

class JavaThreadFactory : public Valdi::PlatformThreadFactory {
public:
    JavaThreadFactory() = default;
    ~JavaThreadFactory() override = default;

    Valdi::PlatformThread* create(const Valdi::StringBox& name,
                                  Valdi::ThreadQoSClass qosClass,
                                  Valdi::Thread* handler) override {
        auto javaEnv = ValdiAndroid::JavaEnv();
        const auto& threadStartMethod = ValdiAndroid::JavaEnv::getCache().getValdiThreadStartMethod();
        const auto& threadClass = ValdiAndroid::JavaEnv::getCache().getValdiThreadClass().getClass();
        auto threadName = ValdiAndroid::toJavaObject(javaEnv, name);

        ScopedJNIExceptionHandler exceptionHandler;
        auto javaThread = threadStartMethod.call(
            threadClass, threadName, static_cast<int32_t>(qosClass), reinterpret_cast<int64_t>(handler));

        if (exceptionHandler) {
            return reinterpret_cast<Valdi::PlatformThread*>(new JavaThread(javaEnv, javaThread.getUnsafeObject()));
        } else {
            auto exceptionMessage = exceptionHandler.getException()->getMessage();
            VALDI_ERROR(Valdi::ConsoleLogger::getLogger(), "Failed to create Java Thread! '{}'", exceptionMessage);

            // If we failed to create the thread during the JVM runtime Thread, we just return nullptr and the thread
            // will not start.
            if (exceptionMessage.find("runtime shutdown") != std::string::npos) {
                exceptionHandler.clear();
            } else {
                exceptionHandler.getException().value().handleFatal(JavaEnv::getUnsafeEnv(), std::nullopt);
            }

            return nullptr;
        }
    }

    void destroy(Valdi::PlatformThread* thread) override {
        auto* javaThread = reinterpret_cast<ValdiAndroid::JavaThread*>(thread);
        auto javaThreadObject = javaThread->toObject();
        ValdiAndroid::JavaEnv::getCache().getValdiThreadJoinMethod().call(javaThreadObject);
        delete javaThread;
    }

    void setQoSClass(Valdi::PlatformThread* thread, Valdi::ThreadQoSClass qosClass) final {
        auto* javaThread = reinterpret_cast<ValdiAndroid::JavaThread*>(thread);
        auto javaThreadObject = javaThread->toObject();
        ValdiAndroid::JavaEnv::getCache().getValdiThreadUpdateQoSMethod().call(javaThreadObject,
                                                                               static_cast<int32_t>(qosClass));
    }
};

JavaThread::JavaThread(JavaEnv env, jobject ref) : GlobalRefJavaObject(env, ref, "JavaThread", false) {}

JavaThread::~JavaThread() = default;

void jniNativeThreadEntryPoint(JNIEnv* /*env*/, jclass /*cls*/, jlong ptr) {
    auto* thread = reinterpret_cast<Valdi::Thread*>(ptr);
    thread->handler();
}

void JavaThread::registerNatives() {
    Valdi::PlatformThreadFactory::set(new JavaThreadFactory());

    facebook::jni::registerNatives("com/snap/valdi/utils/ValdiThread",
                                   {makeNativeMethod("nativeThreadEntryPoint", jniNativeThreadEntryPoint)});
}

} // namespace ValdiAndroid
