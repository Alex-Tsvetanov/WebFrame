//
//  BaseDisplayLinkFrameScheduler.hpp
//  snap_drawing-pc
//
//  Created by Simon Corsin on 1/21/22.
//

#pragma once

#include "snap_drawing/cpp/Drawing/IFrameScheduler.hpp"
#include "snap_drawing/cpp/Utils/Aliases.hpp"
#include "valdi_core/cpp/Interfaces/ILogger.hpp"
#include "valdi_core/cpp/Utils/Mutex.hpp"
#include "valdi_core/cpp/Utils/SmallVector.hpp"

#include <deque>

namespace snap::drawing {

using CallbackQueue = Valdi::SmallVector<Ref<IFrameCallback>, 2>;

/**
 Abstract class for an implementation of IFrameScheduler on Apple platforms
 meant to use a DisplayLink (like CADisplayLink or CVDisplayLink).
 */
class BaseDisplayLinkFrameScheduler : public IFrameScheduler {
public:
    explicit BaseDisplayLinkFrameScheduler(Valdi::ILogger& logger);
    ~BaseDisplayLinkFrameScheduler() override;

    void onNextVSync(const Ref<IFrameCallback>& callback) override;

    void onMainThread(const Ref<IFrameCallback>& callback) override;

    void onVSync();

    Valdi::ILogger& getLogger() const;

protected:
    virtual void onResume(std::unique_lock<Valdi::Mutex>& lock) = 0;
    virtual void onPause(std::unique_lock<Valdi::Mutex>& lock) = 0;

    std::unique_lock<Valdi::Mutex> lock() const;

    void onDisplayLinkChanged(std::unique_lock<Valdi::Mutex>& lock);

private:
    [[maybe_unused]] Valdi::ILogger& _logger;
    snap::drawing::TimePoint _displayLinkTimeout = snap::drawing::TimePoint::now();
    CallbackQueue _vsyncCallbacks;
    CallbackQueue _mainThreadCallbacks;
    mutable Valdi::Mutex _mutex;
    bool _displayLinkRunning = false;

    void updateDisplayLink(std::unique_lock<Valdi::Mutex>& lock);

    size_t flushCallbacks(CallbackQueue& queue, TimePoint time);
    void flushMainThreadCallbacks(TimePoint time);
    void flushVSyncCallbacks(TimePoint time);

    static void mainQueueCallback(void* context);
};

} // namespace snap::drawing
