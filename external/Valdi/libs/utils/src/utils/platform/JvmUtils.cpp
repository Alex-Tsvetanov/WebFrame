#ifdef ANDROID_WITH_JNI

#include "utils/platform/JvmUtils.hpp"

#include "utils/debugging/Assert.hpp"
#include "utils/debugging/Trace.hpp"
#include "utils/platform/JvmOnLoad.hpp"

#include <fmt/format.h>

#include <android/log.h>
#include <cassert>
#include <djinni/jni/djinni_support.hpp>
#include <jni.h>
#include <pthread.h>

// This implementation is based on code from
// https://chromium.googlesource.com/external/webrtc/+/HEAD/sdk/android/src/jni/jvm.cc

namespace snap::utils::platform {

namespace {
// Global JVM reference that was passed by the app
JavaVM* gJvm = nullptr;

// Key for per-thread JNIEnv* data.  Non-NULL in threads attached to |gJvm| by
// AttachCurrentThreadIfNeeded(), NULL in unattached threads and threads that
// were attached by the JVM because of a Java->native call.
pthread_key_t gJniPtr;

void threadDestructor([[maybe_unused]] void* prevJniPtr) {
    // This function only runs on threads where gJniPtr is non-null, meaning
    // we were responsible for originally attaching the thread, so are responsible
    // for detaching it now.  However, because some JVM implementations (notably
    // Oracle's http://goo.gl/eHApYT) also use the pthread_key_create mechanism,
    // the JVMs accounting info for this thread may already be wiped out by the
    // time this is called. Thus it may appear we are already detached even though
    // it was our responsibility to detach!  Oh well.
    if (getEnv() == nullptr)
        return;

    assert(getEnv() == prevJniPtr);
    [[maybe_unused]] jint status = gJvm->DetachCurrentThread();
    SC_ASSERT(status == JNI_OK);
}
} // namespace

// Called when library is loaded by the first class which uses it.
extern "C" JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* jvm, void* reserved) // NOLINT naming
{
    debugging::ScopedTrace trace("client::JNI_OnLoad");
    gJvm = jvm;
    djinni::jniInit(jvm);

    [[maybe_unused]] auto result = pthread_key_create(&gJniPtr, &threadDestructor);
    SC_ASSERT(!result);

    JNIEnv* jni = nullptr;
    if (jvm->GetEnv(reinterpret_cast<void**>(&jni), JNI_VERSION_1_6) != JNI_OK)
        return -1;

    jni::runOnLoadInitializers(jni);

    return JNI_VERSION_1_6;
}

// (Potentially) called when library is about to be unloaded.
extern "C" JNIEXPORT void JNICALL JNI_OnUnload(JavaVM* jvm, void* reserved) // NOLINT naming
{
    gJvm = nullptr;
    djinni::jniShutdown();
}

namespace {

JNIEnv* attachJniInternal(const char* threadName) {
    if (gJvm == nullptr) {
        return nullptr;
    }
    JavaVMAttachArgs args{};
    args.version = JNI_VERSION_1_6;
    args.name = threadName;
    args.group = nullptr;
// Deal with difference in signatures between Oracle's jni.h and Android's.
#ifdef _JAVASOFT_JNI_H_ // Oracle's jni.h violates the JNI spec
    void* env = nullptr;
#else
    JNIEnv* env = nullptr;
#endif
    // this function may fail if JVM is shutting down:
    [[maybe_unused]] auto result = gJvm->AttachCurrentThread(&env, &args);
    assert(result == JNI_OK);
    assert(env != nullptr);

    return static_cast<JNIEnv*>(env);
}
} // namespace

// Return a |JNIEnv*| usable on this thread or NULL if this thread is detached.
JNIEnv* getEnv() {
    void* env = nullptr;
    [[maybe_unused]] jint status = gJvm->GetEnv(&env, JNI_VERSION_1_6);
    assert(((env != nullptr) && (status == JNI_OK)) || ((env == nullptr) && (status == JNI_EDETACHED)));
    return reinterpret_cast<JNIEnv*>(env);
}

bool attachThreadIfNeeded(const char* threadName) {
    // check if already attached
    if (auto* jni = getEnv(); jni != nullptr)
        return true;

    SC_ASSERT(!pthread_getspecific(gJniPtr));
    auto* jni = attachJniInternal(threadName);
    auto result = pthread_setspecific(gJniPtr, jni);
    assert(result == 0);
    return result == 0;
}

void attachJni(const char* threadName) {
    attachJniInternal(threadName);
}

void detachJni() {
    if (gJvm != nullptr) {
        gJvm->DetachCurrentThread();
    }
}

JavaVM* getJvm() {
    return gJvm;
}

// Register java native methods with JNI C methods. Crashes immediately if errors occur.
void registerNativeMethods(JNIEnv* env, const char* className, const JNINativeMethod methods[], int numMethods) {
    jclass clazz;
    clazz = env->FindClass(className);
    if (clazz == nullptr) {
        // This will log and immediately abort() which is what we want, because
        // if we keep going then we'll crash when someone tries to call this
        // unregistered method.
        __android_log_assert(nullptr, "ClientAssert", "Native registration unable to find class '%s'", className);
    }
    int res = env->RegisterNatives(clazz, methods, numMethods);
    if (res < 0) {
        __android_log_assert(nullptr, "ClientAssert", "RegisterNatives failed for '%s'", className);
    }
}

std::string getDeviceModel() {
    JNIEnv* env = getEnv();
    if (env == nullptr) {
        return {};
    }
    jclass buildClass = env->FindClass("android/os/Build");
    jfieldID modelId = env->GetStaticFieldID(buildClass, "MODEL", "Ljava/lang/String;");
    auto* modelObj = reinterpret_cast<jstring>(env->GetStaticObjectField(buildClass, modelId));
    const char* deviceModel = env->GetStringUTFChars(modelObj, nullptr);
    if (env->ExceptionCheck() != 0u) {
        env->ExceptionClear();
        return {};
    }
    return std::string(deviceModel);
}
} // namespace snap::utils::platform

class UncaughtJavaException : public djinni::jni_exception {
private:
    std::string _whatMessage;

public:
    UncaughtJavaException(JNIEnv* env, jthrowable javaException, std::string whatArg)
        : jni_exception(env, javaException), _whatMessage(std::move(whatArg)) {};

    const char* what() const noexcept override {
        return this->_whatMessage.c_str();
    }
};

namespace djinni {

// Overriding the handler that processes exceptions thrown in Java
DJINNI_NORETURN_DEFINITION void jniThrowCppFromJavaException(JNIEnv* env, jthrowable javaException) {
    // WARN: This code is called from djinni so don't use Djinni helper functions
    // here to avoid deadlocks and other issues.

    auto checkJniReturn = [env, javaException](auto o, const char* msg) {
        auto* e = env->ExceptionOccurred();
        if (e) {
            // Nothing we can do if we get an exception in the function that
            // translates exceptions.
            env->ExceptionClear();
        }
        if (e || o == nullptr) {
            throw UncaughtJavaException(env, javaException, fmt::format("failed to translate java exception: {}", msg));
        }
    };
    auto getJniString = [env](jstring jstr) {
        const char* chars = env->GetStringUTFChars(jstr, nullptr);
        if (chars) {
            std::string cstr(chars);
            env->ReleaseStringUTFChars(jstr, chars);
            return cstr;
        } else {
            return std::string("[N/A]");
        }
    };

    jclass arraysClass = env->FindClass("java/util/Arrays");
    checkJniReturn(arraysClass, "unable to find class java.util.Arrays");

    jclass throwableClass = env->FindClass("java/lang/Throwable");
    checkJniReturn(throwableClass, "unable to find class java.lang.Throwable");

    jmethodID arraysToStringMethod =
        env->GetStaticMethodID(arraysClass, "toString", "([Ljava/lang/Object;)Ljava/lang/String;");
    checkJniReturn(arraysToStringMethod, "unable to get method Arrays.toString()");

    jmethodID throwableToStringMethod = env->GetMethodID(throwableClass, "toString", "()Ljava/lang/String;");
    checkJniReturn(throwableToStringMethod, "unable to get method Throwable.toString()");

    jmethodID getStackTraceMethod =
        env->GetMethodID(throwableClass, "getStackTrace", "()[Ljava/lang/StackTraceElement;");
    checkJniReturn(getStackTraceMethod, "unable to get method Throwable.getStackTrace()");

    auto* msgStr = reinterpret_cast<jstring>(env->CallObjectMethod(javaException, throwableToStringMethod));
    checkJniReturn(msgStr, "failed calling Throwable.toString()");

    jobject stack = env->CallObjectMethod(javaException, getStackTraceMethod);
    checkJniReturn(stack, "failed calling Throwable.getStackTrace()");

    auto* stackStr = reinterpret_cast<jstring>(env->CallStaticObjectMethod(arraysClass, arraysToStringMethod, stack));
    checkJniReturn(stackStr, "failed calling Arrays.toString()");

    std::string exceptionMessage =
        fmt::format("Message: [{}], StackTrace: {}", getJniString(msgStr), getJniString(stackStr));

    __android_log_print(
        ANDROID_LOG_ERROR, "[scclient]", "Java exception to propagate to native: %s", exceptionMessage.data());
    throw UncaughtJavaException(env, javaException, std::move(exceptionMessage));
}
} // namespace djinni

#endif
