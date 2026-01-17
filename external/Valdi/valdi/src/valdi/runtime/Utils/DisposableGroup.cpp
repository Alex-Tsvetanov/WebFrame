//
//  DisposableGroup.cpp
//  valdi
//
//  Created by Simon Corsin on 5/14/21.
//

#include "valdi/runtime/Utils/DisposableGroup.hpp"

namespace Valdi {

DisposableGroup::DisposableGroup(Mutex& mutex) : _mutex(mutex) {}

DisposableGroup::~DisposableGroup() {
    std::unique_lock<Mutex> lock(_mutex);
    doDispose(lock);
}

bool DisposableGroup::dispose(std::unique_lock<Mutex>& disposablesLock) {
    return doDispose(disposablesLock);
}

bool DisposableGroup::doDispose(std::unique_lock<Mutex>& disposablesLock) {
    if (_disposed) {
        return false;
    }
    _disposed = true;

    while (!_disposables.empty()) {
        auto it = _disposables.begin();
        auto* disposable = *it;
        _disposables.erase(it);

        if (_listener != nullptr) {
            _listener->onRemove(disposable);
        }

        disposable->dispose(disposablesLock);

        if (!disposablesLock.owns_lock()) {
            disposablesLock.lock();
        }
    }

    return true;
}

bool DisposableGroup::insert(Disposable* disposable) {
    std::unique_lock<Mutex> lock(_mutex);
    if (!_disposed) {
        _disposables.insert(disposable);

        if (_listener != nullptr) {
            _listener->onInsert(disposable);
        }

        return true;
    } else {
        disposable->dispose(lock);
        return false;
    }
}

bool DisposableGroup::remove(Disposable* disposable) {
    std::lock_guard<Mutex> lock(_mutex);
    const auto& it = _disposables.find(disposable);
    if (it == _disposables.end()) {
        return false;
    }
    _disposables.erase(it);

    if (_listener != nullptr) {
        _listener->onRemove(disposable);
    }

    return true;
}

void DisposableGroup::setListener(const Ref<DisposableGroupListener>& listener) {
    std::lock_guard<Mutex> lock(_mutex);
    _listener = listener;
}

std::vector<Disposable*> DisposableGroup::all() const {
    std::unique_lock<Mutex> lock(_mutex);
    return std::vector<Disposable*>(_disposables.begin(), _disposables.end());
}

} // namespace Valdi
