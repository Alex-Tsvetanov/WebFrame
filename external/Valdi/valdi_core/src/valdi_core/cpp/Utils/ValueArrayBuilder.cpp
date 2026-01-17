//
//  ValueArrayBuilder.cpp
//  valdi-pc
//
//  Created by Simon Corsin on 5/13/20.
//

#include "valdi_core/cpp/Utils/ValueArrayBuilder.hpp"
#include "utils/debugging/Assert.hpp"

namespace Valdi {

constexpr size_t kBufferMinSize = 4;

ValueArrayBuilder::ValueArrayBuilder() = default;

ValueArrayBuilder::ValueArrayBuilder(const ValueArrayBuilder& other)
    : _buffer(other._buffer != nullptr ? other._buffer->clone() : nullptr), _size(other._size) {}

void ValueArrayBuilder::prepend(Value&& value) {
    emplace(Value());

    Value lastValue = value;
    for (size_t i = 0; i < _size; i++) {
        auto tmp = (*_buffer)[i];

        (*_buffer)[i] = lastValue;

        lastValue = tmp;
    }
}

void ValueArrayBuilder::append(Value&& value) {
    if (_buffer == nullptr) {
        _buffer = ValueArray::make(kBufferMinSize);
    } else if (_size >= _buffer->size()) {
        auto newBuffer = ValueArray::make(_size * 2);
        newBuffer->copy(_buffer->begin(), _buffer->end());

        _buffer = std::move(newBuffer);
    }

    _buffer->emplace(_size++, std::move(value));
}

void ValueArrayBuilder::append(const Value& value) {
    append(Value(value));
}

void ValueArrayBuilder::remove(size_t index) {
    SC_ASSERT(index < _size);

    for (size_t i = index + 1; i < _size; i++) {
        (*_buffer)[i - 1] = (*_buffer)[i];
    }

    _size--;
    (*_buffer)[_size] = Value();
}

void ValueArrayBuilder::reserve(size_t size) {
    if (capacity() < size) {
        if (_buffer == nullptr) {
            _buffer = ValueArray::make(size);
        } else {
            auto newBuffer = ValueArray::make(size);
            newBuffer->copy(_buffer->begin(), _buffer->end());

            _buffer = std::move(newBuffer);
        }
    }
}

void ValueArrayBuilder::clear() {
    for (size_t i = 0; i < _size; i++) {
        (*_buffer)[i] = Value();
    }

    _size = 0;
}

ValueArrayBuilder& ValueArrayBuilder::operator=(const ValueArrayBuilder& other) {
    if (this != &other) {
        _buffer = other._buffer != nullptr ? other._buffer->clone() : nullptr;
        _size = other._size;
    }

    return *this;
}

const Value& ValueArrayBuilder::operator[](size_t index) const {
    return (*_buffer)[index];
}

size_t ValueArrayBuilder::size() const {
    return _size;
}

size_t ValueArrayBuilder::capacity() const {
    return _buffer != nullptr ? _buffer->size() : 0;
}

bool ValueArrayBuilder::empty() const {
    return _size == 0;
}

const Value* ValueArrayBuilder::begin() const {
    return _buffer != nullptr ? _buffer->begin() : nullptr;
}

const Value* ValueArrayBuilder::end() const {
    return begin() + _size;
}

Ref<ValueArray> ValueArrayBuilder::build() const {
    auto out = ValueArray::make(_size);
    out->copy(_buffer->begin(), _buffer->begin() + _size);
    return out;
}
} // namespace Valdi
