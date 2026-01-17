//
//  MainThreadDispatcher.hpp
//  ValdiAndroidNative
//
//  Created by Simon Corsin on 6/13/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

#pragma once

#include "valdi_core/cpp/Interfaces/IMainThreadDispatcher.hpp"
#include "valdi_core/jni/GlobalRefJavaObject.hpp"

namespace ValdiAndroid {

class MainThreadDispatcher : GlobalRefJavaObjectBase, public Valdi::IMainThreadDispatcher {
public:
    MainThreadDispatcher(JavaEnv env, jobject object);
    ~MainThreadDispatcher() override;

    void dispatchAfter(int64_t delayMs, Valdi::DispatchFunction* function);

    void dispatch(Valdi::DispatchFunction* function, bool sync) override;

private:
    JavaMethod<VoidType, int64_t> _runOnMainThreadMethod;
    JavaMethod<VoidType, int64_t, int64_t> _runOnMainThreadDelayedMethod;
};

} // namespace ValdiAndroid
