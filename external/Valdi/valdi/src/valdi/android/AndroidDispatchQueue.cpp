//
//  AndroidDispatchQueue.cpp
//  valdi-android
//
//  Created by Simon Corsin on 7/13/20.
//

#include "AndroidDispatchQueue.hpp"
#include "utils/debugging/Assert.hpp"
#include "valdi/android/MainThreadDispatcher.hpp"

namespace ValdiAndroid {

AndroidDispatchQueue::AndroidDispatchQueue(const Valdi::Ref<MainThreadDispatcher>& dispatcher)
    : _dispatcher(dispatcher) {}

AndroidDispatchQueue::~AndroidDispatchQueue() {
    _taskQueue.dispose();
}

void AndroidDispatchQueue::sync(const Valdi::DispatchFunction& /*function*/) {
    SC_ABORT("Cannot call sync() on main thread");
}

void AndroidDispatchQueue::async(Valdi::DispatchFunction function) {
    _taskQueue.enqueue(std::move(function));
    scheduleFlush(std::chrono::steady_clock::duration(0));
}

Valdi::task_id_t AndroidDispatchQueue::asyncAfter(Valdi::DispatchFunction function,
                                                  std::chrono::steady_clock::duration delay) {
    auto task = _taskQueue.enqueue(std::move(function), delay);
    scheduleFlush(delay);
    return task.id;
}

void AndroidDispatchQueue::cancel(Valdi::task_id_t taskId) {
    _taskQueue.cancel(taskId);
}

bool AndroidDispatchQueue::isCurrent() const {
    if (!_mainThreadId.has_value()) {
        return false;
    }

    return std::this_thread::get_id() == _mainThreadId.value();
}

void AndroidDispatchQueue::fullTeardown() {
    _taskQueue.dispose();
}

void AndroidDispatchQueue::setListener(const Valdi::Shared<Valdi::IQueueListener>& listener) {
    _taskQueue.setListener(listener);
}

Valdi::Shared<Valdi::IQueueListener> AndroidDispatchQueue::getListener() const {
    return _taskQueue.getListener();
}

void AndroidDispatchQueue::scheduleFlush(std::chrono::steady_clock::duration delay) {
    // TODO(simon): We could avoid scheduling a flush for every tasks.

    auto finish = std::chrono::steady_clock::now() + delay;
    auto* self = Valdi::unsafeRetain(this);
    auto* cb = new Valdi::DispatchFunction([self, finish]() {
        auto remaining = finish - std::chrono::steady_clock::now();
        if (remaining.count() > 0) {
            self->scheduleFlush(remaining);
        } else {
            self->flush();
        }
        Valdi::unsafeRelease(self);
    });

    int64_t delayMs = std::chrono::duration_cast<std::chrono::milliseconds>(delay).count();
    if (delayMs == 0) {
        _dispatcher->dispatch(cb, false);
    } else {
        _dispatcher->dispatchAfter(delayMs, cb);
    }
}

void AndroidDispatchQueue::flush() {
    if (!_mainThreadId.has_value()) {
        _mainThreadId = {std::this_thread::get_id()};
    }
    while (_taskQueue.runNextTask()) {
    }
}

} // namespace ValdiAndroid
