//
//  ValueArray.cpp
//  valdi-pc
//
//  Created by Simon Corsin on 5/13/20.
//

#include "valdi_core/cpp/Utils/ValueArray.hpp"
#include "valdi_core/cpp/Utils/InlineContainerAllocator.hpp"

#include <algorithm>

namespace Valdi {

ValueArray::ValueArray(size_t size) : _size(size) {
    InlineContainerAllocator<ValueArray, Value> allocator;
    for (size_t i = 0; i < size; i++) {
        allocator.constructContainerEntry(this, i);
    }
}

ValueArray::~ValueArray() {
    InlineContainerAllocator<ValueArray, Value> allocator;
    allocator.deallocate(this, _size);
}

size_t ValueArray::size() const {
    return _size;
}

Value* ValueArray::begin() {
    InlineContainerAllocator<ValueArray, Value> allocator;
    return allocator.getContainerStartPtr(this);
}

Value* ValueArray::end() {
    return &begin()[_size];
}

const Value* ValueArray::begin() const {
    InlineContainerAllocator<ValueArray, Value> allocator;
    return allocator.getContainerStartPtr(this);
}

const Value* ValueArray::end() const {
    return &begin()[_size];
}

const Value& ValueArray::operator[](size_t i) const {
    SC_ASSERT(i < size());
    return *(begin() + i);
}

Value& ValueArray::operator[](size_t i) {
    SC_ASSERT(i < size());
    return *(begin() + i);
}

bool ValueArray::operator!=(const ValueArray& other) const {
    return !(*this == other);
}

bool ValueArray::operator==(const ValueArray& other) const {
    if (&other == this) {
        return true;
    }

    if (size() != other.size()) {
        return false;
    }

    const auto* it = begin();
    const auto* otherIt = other.begin();

    while (it != end()) {
        if (*it != *otherIt) {
            return false;
        }
        ++it;
        ++otherIt;
    }

    return true;
}

bool ValueArray::empty() const {
    return _size == 0;
}

Ref<ValueArray> ValueArray::clone() const {
    auto out = make(size());
    out->copy(begin(), end());

    return out;
}

void ValueArray::sort() {
    std::sort(begin(), end());
}

void ValueArray::copy(Value* it, const Value* from, const Value* to) {
    SC_ASSERT((to - from) <= (end() - it));

    while (from != to) {
        *it = *from;

        it++;
        from++;
    }
}

void ValueArray::copy(const Value* from, const Value* to) {
    copy(begin(), from, to);
}

void ValueArray::emplace(size_t index, Value&& value) {
    (*this)[index] = std::move(value);
}

void ValueArray::emplace(size_t index, const Value& value) {
    (*this)[index] = value;
}

Ref<ValueArray> ValueArray::make(const Value* begin, const Value* end) {
    auto array = ValueArray::make(static_cast<size_t>(end - begin));
    array->copy(begin, end);
    return array;
}

Ref<ValueArray> ValueArray::make(size_t size) {
    InlineContainerAllocator<ValueArray, Value> allocator;
    return allocator.allocate(size, size);
}
} // namespace Valdi
