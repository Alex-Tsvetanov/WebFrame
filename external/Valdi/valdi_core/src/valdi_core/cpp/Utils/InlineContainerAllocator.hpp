//
//  InlineContainerAllocator.hpp
//  valdi_core-pc
//
//  Created by Simon Corsin on 1/17/23.
//

#pragma once

#include "valdi_core/cpp/Utils/Shared.hpp"

namespace Valdi {

/**
 Returns a size, that is equal or more than the given size, and compatible
 with the given alignment.
 */
constexpr size_t alignUp(size_t size, size_t alignment) {
    // See details here:
    // https://github.com/KabukiStarship/KabukiToolkit/wiki/Fastest-Method-to-Align-Pointers#observations
    return (size + alignment - 1) & ~(alignment - 1);
}

template<typename T, typename ValueType>
struct InlineContainerAllocator {
    template<typename... Args>
    T* allocateUnmanaged(size_t size, Args&&... args) {
        std::allocator<uint8_t> allocator;

        auto allocSize = alignUp(sizeof(T), alignof(ValueType)) + (sizeof(ValueType) * size);

        auto* arrayRegion = allocator.allocate(allocSize);

        new (arrayRegion)(T)(std::forward<Args>(args)...);

        return reinterpret_cast<T*>(arrayRegion);
    }

    template<typename... Args>
    Ref<T> allocate(size_t size, Args&&... args) {
        auto* instance = allocateUnmanaged(size, std::forward<Args>(args)...);
        return Ref<T>(instance, AdoptRef());
    }

    constexpr inline ValueType* getContainerStartPtr(T* object) {
        return reinterpret_cast<ValueType*>(reinterpret_cast<uint8_t*>(object) +
                                            alignUp(sizeof(T), alignof(ValueType)));
    }

    constexpr inline const ValueType* getContainerStartPtr(const T* object) {
        return reinterpret_cast<const ValueType*>(reinterpret_cast<const uint8_t*>(object) +
                                                  alignUp(sizeof(T), alignof(ValueType)));
    }

    template<typename... Args>
    constexpr inline void constructContainerEntry(T* object, size_t index, Args&&... args) {
        auto& entry = getContainerStartPtr(object)[index];
        new (&entry)(ValueType)(std::forward<Args>(args)...);
    }

    constexpr inline void constructContainerEntries(T* object, size_t size) {
        auto* entry = getContainerStartPtr(object);
        for (size_t i = 0; i < size; i++) {
            new (&entry[i])(ValueType)();
        }
    }

    constexpr void deallocate(T* object, size_t size) {
        auto* ptr = getContainerStartPtr(object);
        for (size_t i = 0; i < size; i++) {
            ptr[i].~ValueType();
        }
    }
};

} // namespace Valdi
