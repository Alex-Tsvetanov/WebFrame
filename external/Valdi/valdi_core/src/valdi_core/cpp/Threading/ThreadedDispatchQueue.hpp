//
//  ThreadedDispatchQueue.hpp
//  ValdiRuntime
//
//  Created by Simon Corsin on 5/28/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

#pragma once

#include "valdi_core/cpp/Interfaces/ILogger.hpp"
#include "valdi_core/cpp/Threading/DispatchQueue.hpp"
#include "valdi_core/cpp/Threading/TaskQueue.hpp"
#include "valdi_core/cpp/Threading/Thread.hpp"
#include <chrono>
#include <string>

namespace Valdi {

class ThreadedDispatchQueue : public DispatchQueue {
public:
    ThreadedDispatchQueue(const StringBox& name, ThreadQoSClass qosClass);
    ~ThreadedDispatchQueue() override;

    void sync(const DispatchFunction& function) final;
    void async(DispatchFunction function) override;
    task_id_t asyncAfter(DispatchFunction function, std::chrono::steady_clock::duration delay) override;
    void cancel(task_id_t taskId) override;

    /**
     * Returns whether the current thread is part of this DispatchQueue.
     */
    bool isCurrent() const override;

    void fullTeardown() override;

    bool isDisposed() const;

    bool hasThreadRunning() const;

    void setListener(const Shared<IQueueListener>& listener) override;

    void setQoSClass(ThreadQoSClass qosClass) final;

    static ThreadedDispatchQueue* getCurrent();

    // For Testing Only
    Shared<IQueueListener> getListener() const override;

    void setDisableSyncCallsInCallingThread(bool disableSyncCallsInCallingThread) override;

private:
    mutable Mutex _mutex;
    Ref<Thread> _thread;
    Ref<TaskQueue> _taskQueue;
    StringBox _name;
    ThreadQoSClass _qosClass;
    bool _disableSyncCallsInCallingThread = false;

    static void handler(ThreadedDispatchQueue* dispatchQueue, const Ref<TaskQueue>& taskQueue);
    void teardown();
    void startThread();
    void teardownThread();
};

} // namespace Valdi
