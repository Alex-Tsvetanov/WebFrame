//
//  ThreadAccessChecker.hpp
//  valdi-core
//
//  Created by Simon Corsin on 4/12/2024
//

#include "utils/base/NonCopyable.hpp"
#include "valdi_core/cpp/Threading/ThreadBase.hpp"
#include <atomic>

namespace Valdi {

class ThreadAccessCheckerGuard;

/**
 * A class to help detecting unsafe multi-threaded accesses.
 * guard() should be called whenever some code is about to
 * use some objects that are not thread safe. guard() will
 * crash whenever it detects concurrent accesses. It is
 * implemented under the hood using an atomic integer that
 * contains the current thread id with the lock. It is allowed
 * to call guard() recursively within the same thread, but it is
 * not allowed for multiple threads to call guard() at the same time.
 */
class ThreadAccessChecker : public snap::NonCopyable {
public:
    ThreadAccessChecker();

    /**
     * Acquires the lock, or abort if the lock was already acquired
     * by a different thread.
     */
    ThreadAccessCheckerGuard guard();

private:
    std::atomic<ThreadId> _threadIdWithCurrentAccess = kThreadIdNull;

    friend ThreadAccessCheckerGuard;
};

class ThreadAccessCheckerGuard : public snap::NonCopyable {
public:
    ThreadAccessCheckerGuard(ThreadAccessChecker& checker, ThreadId previousThreadId);
    ~ThreadAccessCheckerGuard();

private:
    ThreadAccessChecker& _checker;
    ThreadId _previousThreadId;
};

} // namespace Valdi
