//
//  AndroidFrameScheduler.hpp
//  snap_drawing-android
//
//  Created by Simon Corsin on 1/25/22.
//

#pragma once

#include "snap_drawing/cpp/Drawing/IFrameScheduler.hpp"
#include "valdi_core/jni/GlobalRefJavaObject.hpp"
#include "valdi_core/jni/JavaMethod.hpp"

namespace snap::drawing {

class AndroidFrameScheduler : public IFrameScheduler {
public:
    explicit AndroidFrameScheduler(const ValdiAndroid::GlobalRefJavaObjectBase& frameSchedulerJava);
    ~AndroidFrameScheduler() override;

    void onNextVSync(const Ref<IFrameCallback>& callback) override;

    void onMainThread(const Ref<IFrameCallback>& callback) override;

    static void performCallback(int64_t callbackHandle, int64_t frameTimeNanos);

private:
    ValdiAndroid::GlobalRefJavaObjectBase _frameSchedulerJava;
    ValdiAndroid::JavaMethod<ValdiAndroid::VoidType, int64_t> _onNextVSyncMethod;
    ValdiAndroid::JavaMethod<ValdiAndroid::VoidType, int64_t> _onMainThreadMethod;
    ValdiAndroid::JavaMethod<ValdiAndroid::VoidType> _stopMethod;
};

} // namespace snap::drawing
