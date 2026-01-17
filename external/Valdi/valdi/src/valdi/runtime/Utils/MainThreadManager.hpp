//
//  MainThreadManager.hpp
//  valdi-ios
//
//  Created by Simon Corsin on 8/26/18.
//

#pragma once

#include "valdi_core/cpp/Interfaces/ILogger.hpp"
#include "valdi_core/cpp/Interfaces/IMainThreadDispatcher.hpp"
#include "valdi_core/cpp/Threading/TaskQueue.hpp"
#include "valdi_core/cpp/Utils/Function.hpp"
#include "valdi_core/cpp/Utils/ValueFunction.hpp"

#include "utils/base/NonCopyable.hpp"
#include <atomic>
#include <deque>
#include <mutex>
#include <thread>
#include <utility>

namespace Valdi {

class Context;
class MainThreadManager;
class MainThreadBatch : public snap::NonCopyable {
public:
    explicit MainThreadBatch(MainThreadManager* manager);
    MainThreadBatch();
    ~MainThreadBatch();

private:
    MainThreadManager* _manager = nullptr;
};

struct MainThreadTask {
    size_t flushId;
    Ref<Context> context;
    DispatchFunction function;

    MainThreadTask(size_t flushId, const Ref<Context>& context, DispatchFunction&& function);
    ~MainThreadTask();
};

class MainThreadBatchAllowScope {
public:
    MainThreadBatchAllowScope();
    ~MainThreadBatchAllowScope();

    static bool isAllowed();
};

class MainThreadManager : public SimpleRefCountable {
public:
    explicit MainThreadManager(const Ref<IMainThreadDispatcher>& mainThreadDispatcher);
    ~MainThreadManager() override;

    void clearAndTeardown();

    void postInit();

    void dispatch(const Ref<Context>& context, DispatchFunction function);
    void dispatchSync(const Ref<Context>& context, DispatchFunction function);

    void beginBatch();

    void endBatch();

    MainThreadBatch scopedBatch();

    bool currentThreadIsMainThread() const;

    bool hasBatch() const;

    /**
     Schedule a function to be executed when the main thread is idle
     */
    void onIdle(const Ref<ValueFunction>& callback);

    /**
     Mark the current running thread as the main thread.
     By default the MainThreadManager does a dispatch in the given
     IMainThreadDispatcher to resolve the main thread id.
     Calling this function allows the main thread id to be resolved
     immediately.
     */
    void markCurrentThreadIsMainThread();

    bool runNextTask();

private:
    Ref<IMainThreadDispatcher> _mainThreadDispatcher;
    std::atomic<std::thread::id> _mainThreadId;
    std::atomic_bool _tornDown;
    mutable std::mutex _mutex;
    std::deque<MainThreadTask> _pendingTasks;
    std::deque<Ref<ValueFunction>> _onIdleCallbacks;
    int _batchCount = 0;
    size_t _flushIdSequence = 0;
    size_t _tasksSequence = 0;
    size_t _batchFlushId = 0;
    bool _idleFlushScheduled = false;

    static bool shouldAllowBatchFromCurrentThread();

    void flushNextIdleCallback(uint64_t previousTasksSequence);
    void scheduleFlushIdleCallbacks();

    void flushTasksWithId(size_t flushId);
    bool runNextTaskWithId(size_t flushId);
    bool runNextTaskWithId(size_t flushId, std::unique_lock<std::mutex>& lock);
    void doDispatch(const Ref<Context>& context, DispatchFunction&& function, bool sync);
    void scheduleFlush(size_t flushId, bool sync);
};

} // namespace Valdi
