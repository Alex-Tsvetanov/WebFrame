//
//  AssetsManagerTransaction.cpp
//  valdi
//
//  Created by Simon Corsin on 7/7/21.
//

#include "valdi/runtime/Resources/AssetsManagerTransaction.hpp"

namespace Valdi {

AssetsManagerTransaction::AssetsManagerTransaction(std::unique_lock<std::recursive_mutex>&& lock)
    : _lock(std::move(lock)) {}

AssetsManagerTransaction::~AssetsManagerTransaction() = default;

void AssetsManagerTransaction::releaseLock() {
    if (_lock.owns_lock()) {
        _lock.unlock();
    }
}

void AssetsManagerTransaction::acquireLock() {
    if (!_lock.owns_lock()) {
        _lock.lock();
    }
}

void AssetsManagerTransaction::enqueueUpdate(const AssetKey& assetKey) {
    _updates.emplace_back(assetKey);
}

std::optional<AssetKey> AssetsManagerTransaction::dequeueUpdate() {
    if (_updates.empty()) {
        return std::nullopt;
    }

    auto assetKey = *_updates.begin();

    _updates.erase(_updates.begin());

    return {std::move(assetKey)};
}

thread_local AssetsManagerTransaction* kCurrent = nullptr;

AssetsManagerTransaction* AssetsManagerTransaction::current() {
    return kCurrent;
}

void AssetsManagerTransaction::setCurrent(AssetsManagerTransaction* current) {
    kCurrent = current;
}

} // namespace Valdi
