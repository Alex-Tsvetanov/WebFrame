//
//  RefCountableAutoreleasePool.cpp
//  valdi
//
//  Created by Simon Corsin on 2/28/22.
//

#include "valdi/runtime/Utils/RefCountableAutoreleasePool.hpp"
#include "valdi_core/cpp/Constants.hpp"

namespace Valdi {

thread_local RefCountableAutoreleasePool* tCurrent = nullptr;

RefCountableAutoreleasePool::RefCountableAutoreleasePool() : RefCountableAutoreleasePool(true) {}

RefCountableAutoreleasePool::RefCountableAutoreleasePool(bool deferred) : _parent(tCurrent) {
    tCurrent = VALDI_LIKELY(deferred) ? this : nullptr;
}

RefCountableAutoreleasePool::~RefCountableAutoreleasePool() {
    releaseAll();

    tCurrent = _parent;
}

void RefCountableAutoreleasePool::releaseAll() {
    while (_entriesIndex < _entries.size()) {
        auto index = _entriesIndex++;
        Valdi::unsafeBridgeRelease(_entries[index]);
    }
}

void RefCountableAutoreleasePool::release(void* ptr) {
    auto* current = tCurrent;
    if (current != nullptr) {
        current->append(ptr);
    } else {
        Valdi::unsafeBridgeRelease(ptr);
    }
}

void RefCountableAutoreleasePool::append(void* ptr) {
    // If our append will cause a reallocation, but we have some free spots available
    // at the beginning, shift the array and reset the index to avoid the allocation.
    if (_entriesIndex > 0 && _entries.size() + 1 >= _entries.capacity()) {
        _entries.erase(_entries.begin(), _entries.begin() + _entriesIndex);
        _entriesIndex = 0;
    }

    _entries.emplace_back(ptr);
}

size_t RefCountableAutoreleasePool::backingArraySize() const {
    return _entries.size();
}

RefCountableAutoreleasePool* RefCountableAutoreleasePool::current() {
    return tCurrent;
}

RefCountableAutoreleasePool RefCountableAutoreleasePool::makeNonDeferred() {
    return RefCountableAutoreleasePool(false);
}

} // namespace Valdi
