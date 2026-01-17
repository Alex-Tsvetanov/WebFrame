//
//  DisposableGroup.hpp
//  valdi
//
//  Created by Simon Corsin on 5/14/21.
//

#pragma once

#include "valdi/runtime/Utils/Disposable.hpp"
#include "valdi_core/cpp/Utils/FlatSet.hpp"
#include "valdi_core/cpp/Utils/Mutex.hpp"
#include "valdi_core/cpp/Utils/Shared.hpp"
#include <vector>

namespace Valdi {

class DisposableGroupListener : public SimpleRefCountable {
public:
    virtual void onInsert(Disposable* disposable) = 0;
    virtual void onRemove(Disposable* disposable) = 0;
};

class DisposableGroup : public Disposable {
public:
    explicit DisposableGroup(Mutex& mutex);
    ~DisposableGroup() override;

    bool insert(Disposable* disposable);
    bool remove(Disposable* disposable);

    std::vector<Disposable*> all() const;

    void setListener(const Ref<DisposableGroupListener>& listener);

    bool dispose(std::unique_lock<Mutex>& disposablesLock) override;

private:
    Mutex& _mutex;
    FlatSet<Disposable*> _disposables;
    Ref<DisposableGroupListener> _listener;
    bool _disposed = false;

    bool doDispose(std::unique_lock<Mutex>& disposablesLock);
};

} // namespace Valdi
