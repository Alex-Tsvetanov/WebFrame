#include "valdi_core/cpp/Utils/TrackedLock.hpp"

namespace Valdi {

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wthread-safety-analysis"

thread_local TrackedLock* kTopTrackedLock = nullptr;

TrackedLock::TrackedLock() = default;

TrackedLock::TrackedLock(RecursiveMutex& mutex) : _parent(kTopTrackedLock), _mutex(&mutex), _owns(true) {
    kTopTrackedLock = this;

    mutex.lock();
}

TrackedLock::TrackedLock(TrackedLock&& other) noexcept : _mutex(other._mutex), _owns(other._owns) {
    other._mutex = nullptr;
    other._owns = false;
}

TrackedLock::~TrackedLock() {
    kTopTrackedLock = _parent;

    if (_owns) {
        _owns = false;
        _mutex->unlock();
    }
}

void TrackedLock::lock() {
    if (!_owns) {
        _owns = true;
        _mutex->lock();
    }
}

void TrackedLock::unlock() {
    if (_owns) {
        _owns = false;
        _mutex->unlock();
    }
}

void TrackedLock::suspendLock() {
    if (_owns) {
        _suspended = true;
        unlock();
    }
}

void TrackedLock::resumeLock() {
    if (_suspended) {
        _suspended = false;
        lock();
    }
}

bool TrackedLock::owns() const {
    return _owns;
}

TrackedLock& TrackedLock::operator=(TrackedLock&& other) noexcept {
    if (this != &other) {
        if (_owns) {
            _mutex->unlock();
        }

        _mutex = other._mutex;
        _owns = other._owns;

        other._mutex = nullptr;
        other._owns = false;
    }

    return *this;
}

#pragma clang diagnostic pop

DropAllTrackedLocks::DropAllTrackedLocks() {
    auto* current = kTopTrackedLock;
    while (current != nullptr) {
        current->suspendLock();
        current = current->_parent;
    }
}

DropAllTrackedLocks::~DropAllTrackedLocks() {
    auto* current = kTopTrackedLock;
    while (current != nullptr) {
        current->resumeLock();
        current = current->_parent;
    }
}

} // namespace Valdi