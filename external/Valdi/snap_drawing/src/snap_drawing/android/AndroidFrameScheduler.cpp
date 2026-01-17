//
//  AndroidFrameScheduler.cpp
//  snap_drawing-android
//
//  Created by Simon Corsin on 1/25/22.
//

#if __ANDROID__

#include "snap_drawing/android/AndroidFrameScheduler.hpp"

namespace snap::drawing {

AndroidFrameScheduler::AndroidFrameScheduler(const ValdiAndroid::GlobalRefJavaObjectBase& frameSchedulerJava)
    : _frameSchedulerJava(frameSchedulerJava) {
    auto frameSchedulerJavaObject = _frameSchedulerJava.toObject();
    auto cls = frameSchedulerJavaObject.getClass();
    cls.getMethod("onNextVSync", _onNextVSyncMethod);
    cls.getMethod("onMainThread", _onMainThreadMethod);
    cls.getMethod("stop", _stopMethod);
}

AndroidFrameScheduler::~AndroidFrameScheduler() {
    _stopMethod.call(_frameSchedulerJava.toObject());
}

void AndroidFrameScheduler::onNextVSync(const Ref<IFrameCallback>& callback) {
    auto ptr = reinterpret_cast<int64_t>(Valdi::unsafeBridgeRetain(callback.get()));
    _onNextVSyncMethod.call(_frameSchedulerJava.toObject(), ptr);
}

void AndroidFrameScheduler::onMainThread(const Ref<IFrameCallback>& callback) {
    auto ptr = reinterpret_cast<int64_t>(Valdi::unsafeBridgeRetain(callback.get()));
    _onMainThreadMethod.call(_frameSchedulerJava.toObject(), ptr);
}

void AndroidFrameScheduler::performCallback(int64_t callbackHandle, int64_t frameTimeNanos) {
    auto ref = Valdi::unsafeBridge<IFrameCallback>(reinterpret_cast<void*>(callbackHandle));

    if (ref == nullptr) {
        return;
    }

    ref->onFrame(TimePoint::fromNanoSeconds(frameTimeNanos));
}

} // namespace snap::drawing

#endif
