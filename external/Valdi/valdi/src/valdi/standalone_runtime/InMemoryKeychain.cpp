//
//  InMemoryKeychain.cpp
//  valdi-pc
//
//  Created by Simon Corsin on 4/9/20.
//

#include "valdi/standalone_runtime/InMemoryKeychain.hpp"

namespace Valdi {

InMemoryKeychain::~InMemoryKeychain() = default;

PlatformResult InMemoryKeychain::store(const StringBox& key, const Valdi::BytesView& value) {
    std::lock_guard<Mutex> lock(_mutex);
    _blobs[key] = value;
    _updateSequence++;
    return Value();
}

Valdi::BytesView InMemoryKeychain::get(const StringBox& key) {
    std::lock_guard<Mutex> lock(_mutex);
    const auto& it = _blobs.find(key);
    if (it == _blobs.end()) {
        return {};
    }
    return it->second;
}

bool InMemoryKeychain::erase(const StringBox& key) {
    std::lock_guard<Mutex> lock(_mutex);
    const auto& it = _blobs.find(key);
    if (it == _blobs.end()) {
        return false;
    }
    _updateSequence++;
    _blobs.erase(it);
    return true;
}

uint64_t InMemoryKeychain::getUpdateSequence() const {
    std::lock_guard<Mutex> lock(_mutex);
    return _updateSequence;
}

} // namespace Valdi
