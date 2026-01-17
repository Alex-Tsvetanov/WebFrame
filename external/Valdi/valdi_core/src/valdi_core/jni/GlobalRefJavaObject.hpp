//
//  GlobalRefJavaObject.hpp
//  ValdiAndroidNative
//
//  Created by Simon Corsin on 8/6/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

#pragma once

#include "valdi_core/cpp/Utils/ValdiObject.hpp"
#include "valdi_core/cpp/Utils/ValueConvertible.hpp"
#include "valdi_core/jni/IndirectJavaGlobalRef.hpp"
#include "valdi_core/jni/JavaClass.hpp"
#include "valdi_core/jni/JavaMethod.hpp"
#include "valdi_core/jni/JavaObject.hpp"

namespace ValdiAndroid {

class GlobalRefJavaObjectBase : public Valdi::ValueConvertible {
public:
    GlobalRefJavaObjectBase();
    GlobalRefJavaObjectBase(JavaEnv env, jobject object, const char* tag);
    GlobalRefJavaObjectBase(const JavaObject& object, const char* tag);
    GlobalRefJavaObjectBase(const GlobalRefJavaObjectBase& other);
    GlobalRefJavaObjectBase(GlobalRefJavaObjectBase&& other) noexcept;

    ~GlobalRefJavaObjectBase() override;

    djinni::LocalRef<jobject> get() const;
    JavaObject toObject() const;
    JavaEnv getEnv() const;

    GlobalRefJavaObjectBase& operator=(const GlobalRefJavaObjectBase& other);
    GlobalRefJavaObjectBase& operator=(GlobalRefJavaObjectBase&& other) noexcept;

    Valdi::Value toValue() override;

private:
    IndirectJavaGlobalRef _ref;
};

class GlobalRefJavaObject : public Valdi::ValdiObject, public GlobalRefJavaObjectBase {
public:
    GlobalRefJavaObject(JavaEnv env, jobject object, const char* tag);
    GlobalRefJavaObject(JavaEnv env, jobject object, const char* tag, bool checkForAutoDisposable);
    GlobalRefJavaObject(const JavaObject& object, const char* tag, bool checkForAutoDisposable);

    ~GlobalRefJavaObject() override;

    VALDI_CLASS_HEADER(GlobalRefJavaObject)
private:
    bool _isAutoDisposable = false;
};

} // namespace ValdiAndroid
