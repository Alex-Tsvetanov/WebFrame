//
//  AsyncGroup.cpp
//  valdi-ios
//
//  Created by Simon Corsin on 8/27/19.
//

#include "valdi/runtime/Utils/AsyncGroup.hpp"
#include "utils/debugging/Assert.hpp"
#include "valdi_core/cpp/Utils/Shared.hpp"

namespace Valdi {

AsyncGroup::AsyncGroup() = default;
AsyncGroup::~AsyncGroup() = default;

void AsyncGroup::enter() {
    std::lock_guard<Mutex> guard(_mutex);
    _enterCount++;
}

void AsyncGroup::leave() {
    std::unique_lock<Mutex> guard(_mutex);
    doLeave(guard);
}

bool AsyncGroup::leaveIfNotCompleted() {
    std::unique_lock<Mutex> guard(_mutex);
    if (_enterCount == 0) {
        return false;
    }

    doLeave(guard);
    return true;
}

void AsyncGroup::doLeave(std::unique_lock<Mutex>& guard) {
    SC_ASSERT(_enterCount > 0, "Unbalanced enter(), leave() calls");
    _enterCount--;

    if (_enterCount == 0) {
        auto callbacks = std::move(_callbacks);
        guard.unlock();

        for (const auto& callback : callbacks) {
            callback.second();
        }
    }
}

bool AsyncGroup::isCompleted() const {
    std::lock_guard<Mutex> guard(_mutex);
    return _enterCount == 0;
}

void AsyncGroup::notify(DispatchFunction function) {
    std::unique_lock<Mutex> guard(_mutex);
    if (_enterCount == 0) {
        guard.unlock();
        function();
        return;
    }

    lockFreeEnqueueFunction(std::move(function));
}

void AsyncGroup::blockingWait() {
    std::unique_lock<Mutex> guard(_mutex);
    if (_enterCount == 0) {
        return;
    }

    auto cond = Valdi::makeShared<ConditionVariable>();

    while (_enterCount != 0) {
        auto functionId = lockFreeEnqueueFunction([cond]() { cond->notifyAll(); });

        cond->wait(guard);

        lockFreeRemoveFunction(functionId);
    }
}

bool AsyncGroup::blockingWaitWithTimeout(const std::chrono::steady_clock::duration& maxTime) {
    std::unique_lock<Mutex> guard(_mutex);
    if (_enterCount == 0) {
        return true;
    }

    auto cond = makeShared<ConditionVariable>();

    while (_enterCount != 0) {
        auto functionId = lockFreeEnqueueFunction([cond]() { cond->notifyAll(); });

        auto status = cond->waitFor(guard, maxTime);

        lockFreeRemoveFunction(functionId);

        if (status == std::cv_status::timeout) {
            return false;
        }
    }

    return true;
}

int AsyncGroup::lockFreeEnqueueFunction(DispatchFunction function) {
    auto id = ++_callbackSequence;
    _callbacks.emplace_back(id, std::move(function));
    return id;
}

void AsyncGroup::lockFreeRemoveFunction(int id) {
    auto it = _callbacks.begin();
    while (it != _callbacks.end()) {
        if (it->first == id) {
            it = _callbacks.erase(it);
        } else {
            it++;
        }
    }
}

} // namespace Valdi
