//
//  InternedStringImpl.cpp
//  valdi-ios
//
//  Created by Simon Corsin on 1/24/19.
//

#include "valdi_core/cpp/Utils/InternedStringImpl.hpp"
#include "valdi_core/cpp/Utils/InlineContainerAllocator.hpp"
#include "valdi_core/cpp/Utils/StringCache.hpp"

namespace Valdi {

InternedStringImpl::InternedStringImpl(const char* data, size_t length, size_t hash) noexcept
    : _retainCount(1), _length(static_cast<uint32_t>(length)), _hash(hash) {
    InlineContainerAllocator<InternedStringImpl, char> allocator;
    auto* container = allocator.getContainerStartPtr(this);
    std::memcpy(container, data, length);
    // Put null terminator
    container[length] = '\0';
}

InternedStringImpl::~InternedStringImpl() = default;

size_t InternedStringImpl::getHash() const noexcept {
    return _hash;
}

std::string_view InternedStringImpl::toStringView() const noexcept {
    return std::string_view(getCStr(), _length);
}

size_t InternedStringImpl::length() const noexcept {
    return static_cast<size_t>(_length);
}

const char* InternedStringImpl::getCStr() const noexcept {
    InlineContainerAllocator<InternedStringImpl, char> allocator;
    return allocator.getContainerStartPtr(this);
}

void InternedStringImpl::unsafeRetainInner() {
    ++_retainCount;
}

void InternedStringImpl::unsafeReleaseInner() {
    if (--_retainCount == 0) {
        StringCache::getGlobal().removeString(this);
        delete this;
    }
}

long InternedStringImpl::retainCount() const {
    return _retainCount;
}

Ref<InternedStringImpl> InternedStringImpl::lock() {
    auto retainCount = ++_retainCount;
    /**
     If the retainCount is 1, then the InternedStringImpl was at 0 before so it was pending removal
     from the StringCache instance. In this case we return null.
    */
    if (retainCount == 1) {
        _retainCount = 0;
        return nullptr;
    }

    return Ref<InternedStringImpl>(this, AdoptRef());
}

Ref<InternedStringImpl> InternedStringImpl::make(const char* data, size_t length, size_t hash) {
    InlineContainerAllocator<InternedStringImpl, char> allocator;
    return allocator.allocate(length + 1, data, length, hash);
}

} // namespace Valdi
