//
//  JavaObject.cpp
//  ValdiAndroidNative
//
//  Created by Simon Corsin on 6/9/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

#include "valdi_core/jni/JavaObject.hpp"
#include "valdi_core/jni/JavaClass.hpp"
#include "valdi_core/jni/JavaMethod.hpp"
#include "valdi_core/jni/JavaUtils.hpp"
#include <sstream>

namespace ValdiAndroid {

JavaObject::JavaObject(JavaEnv /*vm*/, ::djinni::LocalRef<jobject> object)
    : _value(JavaValue::makeObject(std::move(object))) {}

JavaObject::JavaObject(JavaEnv /*vm*/, ::djinni::GlobalRef<jobject> object)
    : _value(JavaValue::makeObject(std::move(object))) {}

JavaObject::JavaObject(JavaEnv /*vm*/, jobject object) : _value(JavaValue::unsafeMakeObject(object)) {}

JavaObject::JavaObject(JavaEnv /*vm*/) : _value(JavaValue::unsafeMakeObject(nullptr)) {}

JavaObject::JavaObject(const JavaObject& other) noexcept = default;

JavaObject::JavaObject(JavaObject&& other) noexcept : _value(std::move(other._value)) {}

jobject JavaObject::getUnsafeObject() const {
    return _value.getObject();
}

JavaClass JavaObject::getClass() const {
    return JavaClass(getEnv(), JavaEnv::getUnsafeEnv()->GetObjectClass(getUnsafeObject()));
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
JavaEnv JavaObject::getEnv() const {
    return JavaEnv();
}

const JavaValue& JavaObject::getValue() const {
    return _value;
}

JavaValue& JavaObject::getValue() {
    return _value;
}

jobject JavaObject::releaseObject() {
    return _value.releaseObject();
}

bool JavaObject::operator!=(const std::nullptr_t& ptr) const {
    return getUnsafeObject() != ptr;
}

bool JavaObject::operator==(const std::nullptr_t& ptr) const {
    return getUnsafeObject() == ptr;
}

::djinni::LocalRef<jobject> JavaObject::newLocalRef() const {
    return JavaEnv::newLocalRef(getUnsafeObject());
}

::djinni::LocalRef<jobject> JavaObject::stealLocalRef() {
    if (_value.getRefType() == JavaValueRefType::Local) {
        return ::djinni::LocalRef<jobject>(_value.releaseObject());
    } else {
        return newLocalRef();
    }
}

JavaObject& JavaObject::operator=(const JavaObject& other) noexcept = default;

JavaObject& JavaObject::operator=(ValdiAndroid::JavaObject&& other) noexcept {
    if (this != &other) {
        _value = std::move(other._value);
    }

    return *this;
}

bool JavaObject::isNull() const {
    return getUnsafeObject() == nullptr;
}

} // namespace ValdiAndroid
