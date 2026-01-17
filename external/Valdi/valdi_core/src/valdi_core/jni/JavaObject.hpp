//
//  JavaObject.hpp
//  ValdiAndroidNative
//
//  Created by Simon Corsin on 6/9/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

#pragma once

#include "valdi_core/jni/JavaEnv.hpp"
#include "valdi_core/jni/JavaValue.hpp"
#include <jni.h>
#include <string>

namespace ValdiAndroid {

class JavaClass;

class JavaObject {
public:
    JavaObject(JavaEnv vm, ::djinni::LocalRef<jobject> object);
    JavaObject(JavaEnv vm, ::djinni::GlobalRef<jobject> object);
    JavaObject(JavaEnv vm, jobject object);
    explicit JavaObject(JavaEnv vm);
    JavaObject(const JavaObject& other) noexcept;
    JavaObject(JavaObject&& other) noexcept;

    const JavaValue& getValue() const;
    JavaValue& getValue();
    JavaEnv getEnv() const;
    jobject getUnsafeObject() const;

    JavaClass getClass() const;

    jobject releaseObject();

    bool operator!=(const std::nullptr_t& ptr) const;
    bool operator==(const std::nullptr_t& ptr) const;

    bool isNull() const;
    ::djinni::LocalRef<jobject> newLocalRef() const;
    // Steal the backing local ref from the JavaObject, or create
    // a new ref if this object does not represent a local ref
    ::djinni::LocalRef<jobject> stealLocalRef();

    JavaObject& operator=(const JavaObject& other) noexcept;
    JavaObject& operator=(JavaObject&& other) noexcept;

private:
    JavaValue _value;
};

} // namespace ValdiAndroid
