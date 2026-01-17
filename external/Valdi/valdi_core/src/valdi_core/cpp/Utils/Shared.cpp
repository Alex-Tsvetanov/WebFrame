//
//  Shared.cpp
//  valdi-ios
//
//  Created by Simon Corsin on 10/21/19.
//

#include "valdi_core/cpp/Utils/Shared.hpp"

namespace Valdi {

SimpleRefCountable::SimpleRefCountable() : _retainCount(1) {}
SimpleRefCountable::~SimpleRefCountable() = default;

void SimpleRefCountable::unsafeRetainInner() {
    ++_retainCount;
}

void SimpleRefCountable::unsafeReleaseInner() {
    auto result = --_retainCount;

    if (result == 0) {
        delete this;
    }
}

long SimpleRefCountable::retainCount() const {
    return _retainCount.load();
}

NonAtomicRefCountable::NonAtomicRefCountable() : _retainCount(1) {}
NonAtomicRefCountable::~NonAtomicRefCountable() = default;

void NonAtomicRefCountable::unsafeRetainInner() {
    ++_retainCount;
}

void NonAtomicRefCountable::unsafeReleaseInner() {
    auto result = --_retainCount;

    if (result == 0) {
        delete this;
    }
}

long NonAtomicRefCountable::retainCount() const {
    return _retainCount;
}

// This replicates the structure of std::shared_ptr, which is commonly
// implemented by two pointers, one for the consumed ptr, one for the control block.
struct SharedPtrPrivate {
    SharedPtrRefCountable* ptr;
    SharedPtrRefCountable* cntrl;
};

static_assert(sizeof(SharedPtrPrivate) == sizeof(std::shared_ptr<SharedPtrRefCountable>));

// This replicates the structure of std::enable_shared_from_this, which
// is commonly implemented by keeping a weak pointer reference internally.
struct EnableSharedFromThisPrivate {
    SharedPtrPrivate weakPtr;
};

static_assert(sizeof(EnableSharedFromThisPrivate) == sizeof(std::enable_shared_from_this<SharedPtrRefCountable>));

// Reconstruct a shared_ptr<T> instance from SharedPtrRefCountable*
SharedPtrPrivate reconstructSharedPtr(SharedPtrRefCountable* instance) {
    // We cast into our SharedPtrRefCountable to extract the weak ptr it holds.
    return reinterpret_cast<EnableSharedFromThisPrivate*>(
               static_cast<std::enable_shared_from_this<SharedPtrRefCountable>*>(instance))
        ->weakPtr;
}

const Shared<SharedPtrRefCountable>* SharedPtrRefCountable::getInnerSharedPtr() const {
    return reinterpret_cast<const Shared<SharedPtrRefCountable>*>(
        &reinterpret_cast<const EnableSharedFromThisPrivate*>(
             static_cast<const std::enable_shared_from_this<SharedPtrRefCountable>*>(this))
             ->weakPtr);
}

void SharedPtrRefCountable::unsafeRetainInner() {
    SharedPtrPrivate storage;
    new (&storage) std::shared_ptr<SharedPtrRefCountable>(*getInnerSharedPtr());
}

void SharedPtrRefCountable::unsafeReleaseInner() {
    auto storage = reinterpret_cast<EnableSharedFromThisPrivate*>(
                       static_cast<std::enable_shared_from_this<SharedPtrRefCountable>*>(this))
                       ->weakPtr;

    reinterpret_cast<std::shared_ptr<SharedPtrRefCountable>*>(&storage)->~shared_ptr<SharedPtrRefCountable>();
}

void unsafeBridgeRelease(void* bridgedInstance) {
    unsafeRelease(reinterpret_cast<RefCountable*>(bridgedInstance));
}

long SharedPtrRefCountable::retainCount() const {
    return getInnerSharedPtr()->use_count();
}

} // namespace Valdi
