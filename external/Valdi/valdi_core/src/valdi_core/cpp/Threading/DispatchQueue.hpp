//
//  DispatchQueue.hpp
//  valdi-ios
//
//  Created by Simon Corsin on 4/9/19.
//

#pragma once

#include "valdi_core/cpp/Threading/IDispatchQueue.hpp"
#include "valdi_core/cpp/Threading/IQueueListener.hpp"
#include "valdi_core/cpp/Threading/TaskId.hpp"
#include "valdi_core/cpp/Threading/ThreadQoSClass.hpp"
#include "valdi_core/cpp/Utils/Function.hpp"
#include "valdi_core/cpp/Utils/Shared.hpp"
#include "valdi_core/cpp/Utils/StringBox.hpp"
#include <chrono>
#include <string>
#include <thread>

namespace Valdi {

class DispatchQueue : public IDispatchQueue {
public:
    constexpr static task_id_t TaskIDNull = 0;

    explicit DispatchQueue();
    DispatchQueue(const DispatchQueue& other) = delete;
    ~DispatchQueue() override;

    /**
     Call the given function synchronously while checking if the queue is current.
     It is exception safe. Return whether the function executed succesfully.
     */
    bool safeSync(const DispatchFunction& function);

    /**
     * Returns whether the current thread is part of this DispatchQueue.
     */
    virtual bool isCurrent() const = 0;

    bool isRunningSync() const;

    /**
     Synchronously flush the queue and destroy it
     */
    void flushAndTeardown();

    virtual void fullTeardown() = 0;

    virtual void setListener(const Shared<IQueueListener>& listener) = 0;

    virtual void setQoSClass(ThreadQoSClass qosClass);

    static Ref<DispatchQueue> create(const StringBox& name, ThreadQoSClass qosClass);
    // Create a DispatchQueue that is always backed by a single thread.
    static Ref<DispatchQueue> createThreaded(const StringBox& name, ThreadQoSClass qosClass);

    static DispatchQueue* getCurrent();
    static DispatchQueue* getMain();
    static void setMain(const Ref<DispatchQueue>& main);

    // For Testing Only
    virtual Shared<IQueueListener> getListener() const = 0;

    virtual void setDisableSyncCallsInCallingThread(bool disableSyncCallsInCallingThread);

protected:
    bool _runningSync = false;
};

} // namespace Valdi
