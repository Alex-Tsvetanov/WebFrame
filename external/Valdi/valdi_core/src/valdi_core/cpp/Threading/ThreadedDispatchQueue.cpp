//
//  ThreadedDispatchQueue.cpp
//  ValdiRuntime
//
//  Created by Simon Corsin on 5/28/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

#include "valdi_core/cpp/Threading/ThreadedDispatchQueue.hpp"
#include "valdi_core/cpp/Constants.hpp"
#include "valdi_core/cpp/Utils/LoggerUtils.hpp"
#include <condition_variable>
#include <fmt/format.h>
#include <future>
#include <mutex>

#if __APPLE__
#include "TargetConditionals.h"
#include "valdi_core/cpp/Threading/GCDUtils.hpp"
#endif

#if __APPLE__ && DESKTOP_BUILD
// Workaround for cxx_test() buck rule build on the simulator.
// In that case we don't want to believe we are running on iPhone.
#undef TARGET_OS_IPHONE
#endif

#if TARGET_OS_IPHONE
#include <CoreFoundation/CoreFoundation.h>

/**
 * From https://releases.llvm.org/3.1/tools/clang/docs/AutomaticReferenceCounting.html#runtime.objc_autoreleasePoolPush
 */
extern "C" {
void* objc_autoreleasePoolPush(void);
void objc_autoreleasePoolPop(void* pool);
}

#endif

namespace Valdi {

static thread_local ThreadedDispatchQueue* current = nullptr;

ThreadedDispatchQueue::ThreadedDispatchQueue(const StringBox& name, ThreadQoSClass qosClass)
    : _taskQueue(makeShared<TaskQueue>()), _name(name), _qosClass(qosClass), _disableSyncCallsInCallingThread(false) {}

ThreadedDispatchQueue::~ThreadedDispatchQueue() {
    teardown();
}

void ThreadedDispatchQueue::sync(const Valdi::DispatchFunction& function) {
    if (_disableSyncCallsInCallingThread) {
        std::promise<void> promise;
        auto future = promise.get_future();

        async([&function, &promise, this]() {
            _runningSync = true;
            function();

            promise.set_value();
            _runningSync = false;
        });

        future.get();
        return;
    }

    _taskQueue->barrier([&]() {
        auto* previousCurrent = current;
        current = this;
        _runningSync = true;
        function();
        current = previousCurrent;
        _runningSync = false;
    });
}

void ThreadedDispatchQueue::async(Valdi::DispatchFunction function) {
    auto task = _taskQueue->enqueue(std::move(function));

    if (VALDI_UNLIKELY(task.isFirst)) {
        startThread();
    }
}

task_id_t ThreadedDispatchQueue::asyncAfter(Valdi::DispatchFunction function,
                                            std::chrono::steady_clock::duration delay) {
    auto task = _taskQueue->enqueue(std::move(function), delay);

    if (VALDI_UNLIKELY(task.isFirst)) {
        startThread();
    }

    return task.id;
}

void ThreadedDispatchQueue::cancel(Valdi::task_id_t taskId) {
    _taskQueue->cancel(taskId);
}

Shared<IQueueListener> ThreadedDispatchQueue::getListener() const {
    return _taskQueue->getListener();
}

void ThreadedDispatchQueue::teardown() {
    _taskQueue->dispose();
    teardownThread();
}

void ThreadedDispatchQueue::startThread() {
    std::lock_guard<Mutex> guard(_mutex);
    SC_ASSERT(_thread == nullptr);

    auto threadResult = Thread::create(
        _name, _qosClass, [self = this, taskQueue = _taskQueue]() { ThreadedDispatchQueue::handler(self, taskQueue); });
    SC_ASSERT(threadResult.success(), threadResult.description());

    _thread = threadResult.moveValue();
}

void ThreadedDispatchQueue::teardownThread() {
    std::lock_guard<Mutex> guard(_mutex);

    if (_thread != nullptr) {
        if (this != ThreadedDispatchQueue::getCurrent()) {
            _thread->join();
        }

        _thread = nullptr;
    }
}

void ThreadedDispatchQueue::fullTeardown() {
    teardown();
}

ThreadedDispatchQueue* ThreadedDispatchQueue::getCurrent() {
    return current;
}

void ThreadedDispatchQueue::handler(ThreadedDispatchQueue* dispatchQueue, const Ref<TaskQueue>& taskQueue) {
    current = dispatchQueue;

    while (!taskQueue->isDisposed()) {
#if TARGET_OS_IPHONE
        auto pool = objc_autoreleasePoolPush();

        // On Apple systems, every thread (even C++ threads) come with a CFRunLoop attached.
        // Some system libraries, like JavaScriptCore pushes timers onto the CFRunLoop.
        // We therefore need to flush manually our CFRunLoop as well as our TaskQueue.

        auto nextFireDate = CFRunLoopGetNextTimerFireDate(CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
        if (nextFireDate == 0.0) {
            // Nothing in the run loop, we can loop semi-indefinitely.
            taskQueue->runNextTask(std::chrono::steady_clock::now() + std::chrono::seconds(10000000));
        } else {
            auto timeToWaitSeconds = std::max(nextFireDate - CFAbsoluteTimeGetCurrent(), 0.0);
            // We have something in the runloop, we need to cap our wait to the next task in the CFRunLoop.
            taskQueue->runNextTask(std::chrono::steady_clock::now() +
                                   std::chrono::nanoseconds(static_cast<long long>(timeToWaitSeconds * 1000000000)));
        }

        // Flush the CFRunLoop
        while (CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0, true) == kCFRunLoopRunHandledSource) {
        }

        objc_autoreleasePoolPop(pool);
#else
        taskQueue->runNextTask(std::chrono::steady_clock::now() + std::chrono::seconds(100000));
#endif
    }
}

bool ThreadedDispatchQueue::isCurrent() const {
    return this == getCurrent();
}

bool ThreadedDispatchQueue::isDisposed() const {
    return _taskQueue->isDisposed();
}

bool ThreadedDispatchQueue::hasThreadRunning() const {
    std::lock_guard<Mutex> guard(_mutex);
    return _thread != nullptr;
}

void ThreadedDispatchQueue::setListener(const Shared<IQueueListener>& listener) {
    _taskQueue->setListener(listener);
}

void ThreadedDispatchQueue::setQoSClass(ThreadQoSClass qosClass) {
    std::lock_guard<Mutex> guard(_mutex);
    _qosClass = qosClass;
    if (_thread != nullptr) {
        _thread->setQoSClass(qosClass);
    }
}

void ThreadedDispatchQueue::setDisableSyncCallsInCallingThread(bool disableSyncCallsInCallingThread) {
    _disableSyncCallsInCallingThread = disableSyncCallsInCallingThread;
}

} // namespace Valdi
