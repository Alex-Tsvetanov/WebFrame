//
//  BaseDisplayLinkFrameScheduler.cpp
//  snap_drawing-pc
//
//  Created by Simon Corsin on 1/21/22.
//

#include "utils/platform/TargetPlatform.hpp"

#if __APPLE__
#include "dispatch/dispatch.h"
#include "snap_drawing/apple/Drawing/BaseDisplayLinkFrameScheduler.hpp"
#include "valdi_core/cpp/Utils/LoggerUtils.hpp"

namespace snap::drawing {

/**
 Delay in seconds after which the display link will be set to pause
 if the scheduler did not receive any tasks in that time frame.
 */
constexpr snap::drawing::TimeInterval kDisplayLinkTimeout = 1.0;

struct MainQueueFlushTask {
    TimePoint time;
    Ref<BaseDisplayLinkFrameScheduler> frameScheduler;

    MainQueueFlushTask(TimePoint time, Ref<BaseDisplayLinkFrameScheduler>&& frameScheduler)
        : time(time), frameScheduler(std::move(frameScheduler)) {}
};

BaseDisplayLinkFrameScheduler::BaseDisplayLinkFrameScheduler(Valdi::ILogger& logger) : _logger(logger) {}

BaseDisplayLinkFrameScheduler::~BaseDisplayLinkFrameScheduler() {}

std::unique_lock<Valdi::Mutex> BaseDisplayLinkFrameScheduler::lock() const {
    return std::unique_lock<Valdi::Mutex>(_mutex);
}

void BaseDisplayLinkFrameScheduler::onNextVSync(const Ref<IFrameCallback>& callback) {
    auto guard = lock();
    _vsyncCallbacks.emplace_back(callback);
    updateDisplayLink(guard);
}

void BaseDisplayLinkFrameScheduler::onMainThread(const Ref<IFrameCallback>& callback) {
    auto guard = lock();
    _mainThreadCallbacks.emplace_back(callback);
    updateDisplayLink(guard);
}

void BaseDisplayLinkFrameScheduler::flushMainThreadCallbacks(TimePoint time) {
    auto flushedCallbacksCount = flushCallbacks(_mainThreadCallbacks, time);

    auto guard = lock();

    if (flushedCallbacksCount > 0) {
        _displayLinkTimeout = snap::drawing::TimePoint::now() + snap::drawing::Duration(kDisplayLinkTimeout);
    }

    updateDisplayLink(guard);
}

void BaseDisplayLinkFrameScheduler::flushVSyncCallbacks(TimePoint time) {
    flushCallbacks(_vsyncCallbacks, time);
}

size_t BaseDisplayLinkFrameScheduler::flushCallbacks(CallbackQueue& queue, TimePoint time) {
    CallbackQueue callbacksToFlush;
    {
        std::lock_guard<Valdi::Mutex> lock(_mutex);

        callbacksToFlush = std::move(queue);
        queue.clear();
    }

    for (const auto& callback : callbacksToFlush) {
        callback->onFrame(time);
    }

    return callbacksToFlush.size();
}

void BaseDisplayLinkFrameScheduler::onDisplayLinkChanged(std::unique_lock<Valdi::Mutex>& lock) {
    _displayLinkRunning = false;
    updateDisplayLink(lock);
}

void BaseDisplayLinkFrameScheduler::updateDisplayLink(std::unique_lock<Valdi::Mutex>& lock) {
    auto running = _displayLinkRunning;

    if (_vsyncCallbacks.empty() && _mainThreadCallbacks.empty()) {
        if (running && snap::drawing::TimePoint::now() > _displayLinkTimeout) {
            _displayLinkRunning = false;
            onPause(lock);
        }
    } else {
        if (!running) {
            _displayLinkRunning = true;
            onResume(lock);
        }
    }
}

void BaseDisplayLinkFrameScheduler::onVSync() {
    auto time = TimePoint::now();

    auto flushTask = new MainQueueFlushTask(time, Valdi::strongSmallRef(this));

    dispatch_async_f(dispatch_get_main_queue(), flushTask, &BaseDisplayLinkFrameScheduler::mainQueueCallback);

    flushVSyncCallbacks(time);
}

Valdi::ILogger& BaseDisplayLinkFrameScheduler::getLogger() const {
    return _logger;
}

void BaseDisplayLinkFrameScheduler::mainQueueCallback(void* context) {
    auto task = reinterpret_cast<MainQueueFlushTask*>(context);
    task->frameScheduler->flushMainThreadCallbacks(task->time);
    delete task;
}

} // namespace snap::drawing

#endif
