//
//  AndroidDispatchQueue.hpp
//  valdi-android
//
//  Created by Simon Corsin on 7/13/20.
//

#pragma once

#include "valdi_core/cpp/Threading/DispatchQueue.hpp"
#include "valdi_core/cpp/Threading/TaskQueue.hpp"
#include "valdi_core/cpp/Utils/Mutex.hpp"

#include <optional>

namespace ValdiAndroid {

class MainThreadDispatcher;

class AndroidDispatchQueue : public Valdi::DispatchQueue {
public:
    AndroidDispatchQueue(const Valdi::Ref<MainThreadDispatcher>& dispatcher);
    ~AndroidDispatchQueue() override;

    void sync(const Valdi::DispatchFunction& function) override;
    void async(Valdi::DispatchFunction function) override;
    Valdi::task_id_t asyncAfter(Valdi::DispatchFunction function, std::chrono::steady_clock::duration delay) override;
    void cancel(Valdi::task_id_t taskId) override;

    bool isCurrent() const override;

    void fullTeardown() override;

    void setListener(const Valdi::Shared<Valdi::IQueueListener>& listener) override;

    // For Testing Only
    Valdi::Shared<Valdi::IQueueListener> getListener() const override;

private:
    Valdi::Ref<MainThreadDispatcher> _dispatcher;
    Valdi::TaskQueue _taskQueue;
    std::optional<std::thread::id> _mainThreadId;

    void scheduleFlush(std::chrono::steady_clock::duration delay);
    void flush();
};

} // namespace ValdiAndroid
