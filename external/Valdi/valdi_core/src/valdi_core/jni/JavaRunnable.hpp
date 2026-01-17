//
//  JavaRunnable.hpp
//  valdi_core-android
//
//  Created by Simon Corsin on 4/19/22.
//

#pragma once

#include "valdi_core/cpp/Utils/Shared.hpp"
#include "valdi_core/jni/GlobalRefJavaObject.hpp"

namespace ValdiAndroid {

class JavaRunnable : public GlobalRefJavaObject {
public:
    JavaRunnable(JavaEnv env, jobject jobject);
    ~JavaRunnable() override;

    void operator()();
};

}; // namespace ValdiAndroid
