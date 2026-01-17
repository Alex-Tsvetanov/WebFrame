#pragma once

#include "utils/platform/JNITypeStubs.hpp"
#include <stdexcept>

namespace snap::utils::platform {
// Attaches JNI environment to the currrently executing thread if not set.
// This is necessary when calling from pure C++ thread into Java.
// Returns true if attached successfully.
bool attachThreadIfNeeded(const char* threadName);

// Attaches JNI on Android. Does nothing on other platforms.
void attachJni(const char* threadName);

// Detaches JNI on Android. Does nothing on other platforms.
void detachJni();

// Returns pointer to JVM instance
JavaVM* getJvm();

/**
 * Gets the JNIEnv on Android. Will assert fail on other platforms.
 */
JNIEnv* getEnv();

// Registers JNI methods so they can be called from Java. Will assert fail on other platforms.
void registerNativeMethods(JNIEnv* env, const char* className, const JNINativeMethod methods[], int numMethods);

struct ScopedJniAttach final {
    explicit ScopedJniAttach(const char* threadName) {
        attachJni(threadName);
    }

    ~ScopedJniAttach() {
        detachJni();
    }
};

// move to common header if we later need this on iOS
std::string getDeviceModel();

} // namespace snap::utils::platform
