//
//  JavaEnv.hpp
//  ValdiAndroidNative
//
//  Created by Simon Corsin on 6/13/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

#pragma once

#include "valdi_core/cpp/Utils/Function.hpp"
#include <djinni/jni/djinni_support.hpp>
#include <jni.h>

namespace ValdiAndroid {

class JavaObject;
class JavaClass;
class JavaCache;
class JavaException;

class JNIExceptionHandler {
public:
    virtual ~JNIExceptionHandler() {}

    virtual void onException(JavaException exception) = 0;
};

class JavaEnv {
public:
    JavaEnv();

    static void setExceptionHandlerForCurrentThread(JNIExceptionHandler* exceptionHandler);
    static JNIExceptionHandler* getExceptionHandlerForCurrentThread();

    static JNIEnv* getUnsafeEnv();

    template<typename Fun>
    static auto accessEnv(Fun&& f) {
        f(*getUnsafeEnv());
        checkException();
    }

    template<typename Fun>
    static auto accessEnvRet(Fun&& f) {
        auto result = f(*getUnsafeEnv());
        checkException();
        return result;
    }

    template<typename Fun>
    static auto accessEnvRetRef(Fun&& f) -> ::djinni::LocalRef<decltype(f(*getUnsafeEnv()))> {
        auto result = f(*getUnsafeEnv());
        if (result == nullptr) {
            checkException();
        }
        return ::djinni::LocalRef<decltype(f(*getUnsafeEnv()))>(result);
    }

    static bool checkException();

    static JavaCache& getCache();

    static ::djinni::GlobalRef<jobject> newGlobalRef(jobject obj);
    static ::djinni::GlobalRef<jclass> newGlobalRef(jclass cls);
    static ::djinni::LocalRef<jobject> newLocalRef(jobject obj);

    static bool instanceOf(jobject object, const JavaClass& cls);
    static bool isSameObject(jobject object, jobject otherObject);
};

} // namespace ValdiAndroid
