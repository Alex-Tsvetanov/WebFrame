//
//  AutoMalloc.hpp
//  valdi_core
//
//  Created by Simon Corsin on 11/8/22.
//

#pragma once

#include "utils/base/NonCopyable.hpp"
#include <memory>
#include <type_traits>

namespace Valdi {

template<typename T>
class AutoMallocBase : public snap::NonCopyable {
public:
    AutoMallocBase() = default;
    AutoMallocBase(size_t size, size_t inlineCapacity, T* inlineStorage) : _size(size) {
        allocateData(size, inlineCapacity, inlineStorage);
    }

    ~AutoMallocBase() {
        deallocateData();
    }

    T* data() const {
        return _data;
    }

    bool allocated() const {
        return _allocated;
    }

    size_t size() const {
        return _size;
    }

    size_t capacity() const {
        return _capacity;
    }

protected:
    void performResize(size_t size, size_t inlineCapacity, T* inlineStorage) {
        if (size >= _capacity) {
            deallocateData();
            allocateData(size, inlineCapacity, inlineStorage);
        }

        _size = size;
    }

private:
    T* _data = nullptr;
    bool _allocated = false;
    size_t _size = 0;
    size_t _capacity = 0;

    void deallocateData() {
        if (_allocated) {
            _allocated = false;

            std::allocator<T> allocator;
            allocator.deallocate(_data, _size);
            _data = nullptr;
            _size = 0;
        }
    }

    void allocateData(size_t size, size_t inlineCapacity, T* inlineStorage) {
        if (size < inlineCapacity) {
            _data = inlineStorage;
            _allocated = false;
            _capacity = inlineCapacity;
        } else {
            std::allocator<T> allocator;
            _data = allocator.allocate(size);
            _allocated = true;
            _capacity = size;
        }
    }
};

/**
 AutoMalloc is a container that holds an inline storage on the stack
 and allocates on the heap when the size goes above the inline storage capacity.
 It can serve as a temporary buffer to avoid heap allocations when converting
 arrays of objects that are typically small.
 */
template<typename T, size_t InlineCapacity>
class AutoMalloc : public AutoMallocBase<T> {
public:
    AutoMalloc() = default;
    explicit AutoMalloc(size_t size) : AutoMallocBase<T>(size, InlineCapacity, getInlineStorage()) {}

    void resize(size_t size) {
        AutoMallocBase<T>::performResize(size, InlineCapacity, getInlineStorage());
    }

private:
    typename std::aligned_storage<sizeof(T) * InlineCapacity, alignof(T)>::type _storage;

    constexpr T* getInlineStorage() {
        return reinterpret_cast<T*>(&_storage);
    }
};

} // namespace Valdi
