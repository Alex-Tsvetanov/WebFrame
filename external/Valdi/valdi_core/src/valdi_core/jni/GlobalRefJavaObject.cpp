//
//  GlobalRefJavaObject.cpp
//  ValdiAndroidNative
//
//  Created by Simon Corsin on 8/6/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

#include "valdi_core/jni/GlobalRefJavaObject.hpp"
#include "valdi_core/jni/JavaCache.hpp"
#include "valdi_core/jni/JavaUtils.hpp"
#include <atomic>

namespace ValdiAndroid {

GlobalRefJavaObjectBase::GlobalRefJavaObjectBase() = default;

GlobalRefJavaObjectBase::GlobalRefJavaObjectBase(JavaEnv /*env*/, jobject object, const char* tag)
    : _ref(IndirectJavaGlobalRef::make(object, tag)) {}

GlobalRefJavaObjectBase::GlobalRefJavaObjectBase(const JavaObject& object, const char* tag)
    : GlobalRefJavaObjectBase(object.getEnv(), object.getUnsafeObject(), tag) {}

GlobalRefJavaObjectBase::GlobalRefJavaObjectBase(const GlobalRefJavaObjectBase& other) : _ref(other._ref) {}

GlobalRefJavaObjectBase::GlobalRefJavaObjectBase(GlobalRefJavaObjectBase&& other) noexcept
    : _ref(std::move(other._ref)) {}

GlobalRefJavaObjectBase::~GlobalRefJavaObjectBase() = default;

GlobalRefJavaObjectBase& GlobalRefJavaObjectBase::operator=(const GlobalRefJavaObjectBase& other) {
    if (this != &other) {
        _ref = other._ref;
    }

    return *this;
}

GlobalRefJavaObjectBase& GlobalRefJavaObjectBase::operator=(GlobalRefJavaObjectBase&& other) noexcept {
    if (this != &other) {
        _ref = std::move(other._ref);
    }
    return *this;
}

Valdi::Value GlobalRefJavaObjectBase::toValue() {
    return ValdiAndroid::toValue(JavaEnv(), _ref.get());
}

djinni::LocalRef<jobject> GlobalRefJavaObjectBase::get() const {
    return _ref.get();
}

JavaObject GlobalRefJavaObjectBase::toObject() const {
    return JavaObject(JavaEnv(), get());
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
JavaEnv GlobalRefJavaObjectBase::getEnv() const {
    return JavaEnv();
}

GlobalRefJavaObject::GlobalRefJavaObject(JavaEnv env, jobject object, const char* tag)
    : GlobalRefJavaObject(env, object, tag, true) {}

GlobalRefJavaObject::GlobalRefJavaObject(JavaEnv env, jobject object, const char* tag, bool checkForAutoDisposable)
    : GlobalRefJavaObjectBase(env, object, tag) {
    if (checkForAutoDisposable && object != nullptr &&
        JavaEnv::instanceOf(object, ValdiAndroid::JavaEnv::getCache().getAutoDisposableClass())) {
        ValdiAndroid::JavaEnv::getCache().getAutoDisposableRetainMethod().call(object);
        _isAutoDisposable = true;
    }
}

GlobalRefJavaObject::GlobalRefJavaObject(const JavaObject& object, const char* tag, bool checkForAutoDisposable)
    : GlobalRefJavaObject(object.getEnv(), object.getUnsafeObject(), tag, checkForAutoDisposable) {}

GlobalRefJavaObject::~GlobalRefJavaObject() {
    if (_isAutoDisposable) {
        ValdiAndroid::JavaEnv::getCache().getAutoDisposableReleaseMethod().call(JavaObject(JavaEnv(), get()));
    }
}

VALDI_CLASS_IMPL(GlobalRefJavaObject)

} // namespace ValdiAndroid
