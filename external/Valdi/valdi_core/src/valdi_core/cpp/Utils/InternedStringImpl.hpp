//
//  InternedStringImpl.hpp
//  valdi-ios
//
//  Created by Simon Corsin on 1/24/19.
//

#pragma once

#include "valdi_core/cpp/Utils/Shared.hpp"
#include <memory>
#include <string>

namespace Valdi {

class StringCache;

template<typename T, typename ValueType>
struct InlineContainerAllocator;

/**
 A wrapper around a string always allocated on the heap.
 */
class InternedStringImpl : public RefCountable {
public:
    ~InternedStringImpl() override;

    InternedStringImpl(const InternedStringImpl& other) = delete;
    InternedStringImpl& operator=(InternedStringImpl const&) = delete;

    std::string_view toStringView() const noexcept;
    size_t length() const noexcept;
    const char* getCStr() const noexcept;

    size_t getHash() const noexcept;

    void unsafeRetainInner() final;
    void unsafeReleaseInner() final;

    long retainCount() const;

private:
    std::atomic_uint32_t _retainCount;
    uint32_t _length;
    size_t _hash;

    friend StringCache;
    friend InlineContainerAllocator<InternedStringImpl, char>;

    InternedStringImpl(const char* data, size_t length, size_t hash) noexcept;

    /**
     Returns a reference to this InternedStringImpl if the
     InternedStringImpl is alive and is not pending removal.
     This should be called only with the StringCache mutex locked.
    */
    Ref<InternedStringImpl> lock();

    static Ref<InternedStringImpl> make(const char* data, size_t length, size_t hash);
};

} // namespace Valdi
