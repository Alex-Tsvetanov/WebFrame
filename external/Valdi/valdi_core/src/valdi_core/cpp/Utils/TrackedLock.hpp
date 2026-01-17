#pragma once

#include "valdi_core/cpp/Utils/Mutex.hpp"

namespace Valdi {

class DropAllTrackedLocks;

/**
A lock implementation for Valdi::RecursiveMutex which uses a thread local storage
to track all the locks acquired so that it can unlock them.
 */
class TrackedLock {
public:
    TrackedLock();
    TrackedLock(RecursiveMutex& mutex);
    TrackedLock(TrackedLock&& other) noexcept;

    ~TrackedLock();

    void lock();
    void unlock();

    bool owns() const;

    TrackedLock& operator=(TrackedLock&& other) noexcept;

private:
    friend DropAllTrackedLocks;

    TrackedLock* _parent = nullptr;
    RecursiveMutex* _mutex = nullptr;
    bool _owns = false;
    bool _suspended = false;

    void suspendLock();
    void resumeLock();
};

class DropAllTrackedLocks {
public:
    DropAllTrackedLocks();
    ~DropAllTrackedLocks();
};

} // namespace Valdi