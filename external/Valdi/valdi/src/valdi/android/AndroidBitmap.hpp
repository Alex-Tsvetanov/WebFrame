//
//  AndroidBitmap.cpp
//  ValdiAndroidNative
//
//  Created by Simon Corsin on 8/10/20.
//  Copyright © 2020 Snap Inc. All rights reserved.
//

#pragma once

#include "valdi_core/cpp/Interfaces/IBitmap.hpp"
#include "valdi_core/cpp/Utils/Result.hpp"
#include "valdi_core/jni/GlobalRefJavaObject.hpp"

namespace ValdiAndroid {

class AndroidBitmap : public Valdi::IBitmap {
public:
    explicit AndroidBitmap(const JavaObject& bitmap);
    ~AndroidBitmap() override;

    void dispose() override;

    Valdi::BitmapInfo getInfo() const override;

    void* lockBytes() override;

    void unlockBytes() override;

    JavaObject getJavaBitmap() const;

    static Valdi::Result<Valdi::Ref<AndroidBitmap>> make(const Valdi::BitmapInfo& info);

private:
    GlobalRefJavaObjectBase _bitmap;
};

class AndroidBitmapHandler : public AndroidBitmap {
public:
    AndroidBitmapHandler(JavaEnv env, jobject bitmapHandler, bool isOwned);
    ~AndroidBitmapHandler() override;

private:
    GlobalRefJavaObjectBase _bitmapHandlerRef;
    bool _isOwned;
};

} // namespace ValdiAndroid
