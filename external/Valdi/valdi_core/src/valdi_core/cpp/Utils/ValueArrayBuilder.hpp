//
//  ValueArrayBuilder.hpp
//  valdi-pc
//
//  Created by Simon Corsin on 5/13/20.
//

#pragma once

#include "valdi_core/cpp/Utils/Value.hpp"
#include "valdi_core/cpp/Utils/ValueArray.hpp"

namespace Valdi {

class ValueArrayBuilder {
public:
    ValueArrayBuilder();
    ValueArrayBuilder(const ValueArrayBuilder& other);

    void prepend(Value&& value);
    void append(Value&& value);
    void append(const Value& value);
    void remove(size_t index);
    void clear();

    void reserve(size_t size);

    template<typename T>
    void emplace(T&& value) {
        append(Value(std::forward<T>(value)));
    }

    const Value* begin() const;
    const Value* end() const;

    ValueArrayBuilder& operator=(const ValueArrayBuilder& other);

    const Value& operator[](size_t i) const;

    size_t size() const;
    size_t capacity() const;
    bool empty() const;

    Ref<ValueArray> build() const;

private:
    Ref<ValueArray> _buffer;
    size_t _size = 0;
};

} // namespace Valdi
