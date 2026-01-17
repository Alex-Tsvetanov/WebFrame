//
//  DispatchQueue.cpp
//  valdi-ios
//
//  Created by Simon Corsin on 4/9/19.
//

#include "valdi_core/cpp/Threading/DispatchQueue.hpp"
#include "valdi_core/cpp/Threading/ThreadedDispatchQueue.hpp"
#include <future>

#if __APPLE__
#include "valdi_core/cpp/Threading/GCDDispatchQueue.hpp"
#endif

namespace Valdi {

DispatchQueue::DispatchQueue() = default;

DispatchQueue::~DispatchQueue() = default;

bool DispatchQueue::safeSync(const DispatchFunction& function) {
    if (isCurrent()) {
        function();
    } else {
        sync(function);
    }
    return true;
}

void DispatchQueue::flushAndTeardown() {
    safeSync([this]() { this->fullTeardown(); });
}

bool DispatchQueue::isRunningSync() const {
    return _runningSync;
}

Ref<DispatchQueue> DispatchQueue::createThreaded(const StringBox& name, ThreadQoSClass qosClass) {
    return Valdi::makeShared<ThreadedDispatchQueue>(name, qosClass);
}

void DispatchQueue::setQoSClass(ThreadQoSClass qosClass) {}

void DispatchQueue::setDisableSyncCallsInCallingThread(bool disableSyncCallsInCallingThread) {}

static DispatchQueue* kMain = nullptr;

void DispatchQueue::setMain(const Ref<DispatchQueue>& main) {
    unsafeRelease(kMain);
    kMain = unsafeRetain(main.get());
}

DispatchQueue* DispatchQueue::getMain() {
    return kMain;
}

#if __APPLE__

Ref<DispatchQueue> DispatchQueue::create(const StringBox& name, ThreadQoSClass qosClass) {
    return Valdi::makeShared<GCDDispatchQueue>(name, qosClass);
}

DispatchQueue* DispatchQueue::getCurrent() {
    auto queue = GCDDispatchQueue::getCurrent();
    if (queue != nullptr) {
        return queue;
    }

    return ThreadedDispatchQueue::getCurrent();
}

#else

Ref<DispatchQueue> DispatchQueue::create(const StringBox& name, ThreadQoSClass qosClass) {
    return createThreaded(name, qosClass);
}

DispatchQueue* DispatchQueue::getCurrent() {
    return ThreadedDispatchQueue::getCurrent();
}

#endif

} // namespace Valdi
