//
//  ValueArray.hpp
//  valdi-pc
//
//  Created by Simon Corsin on 5/13/20.
//

#pragma once

#include "utils/base/NonCopyable.hpp"
#include "valdi_core/cpp/Utils/Shared.hpp"
#include "valdi_core/cpp/Utils/Value.hpp"
#include <initializer_list>
#include <utility>
#include <vector>

namespace Valdi {

class Value;

template<typename T, typename ValueType>
struct InlineContainerAllocator;

class ValueArray : public SimpleRefCountable, public snap::NonCopyable {
public:
    ~ValueArray() override;

    size_t size() const;

    Value* begin();
    Value* end();

    const Value* begin() const;
    const Value* end() const;

    bool empty() const;

    void emplace(size_t index, Value&& value);
    void emplace(size_t index, const Value& value);

    const Value& operator[](size_t i) const;
    Value& operator[](size_t i);

    bool operator==(const ValueArray& other) const;
    bool operator!=(const ValueArray& other) const;

    Ref<ValueArray> clone() const;
    void copy(const Value* from, const Value* to);
    void copy(Value* it, const Value* from, const Value* to);

    void sort();

    static Ref<ValueArray> make(size_t size);

    static Ref<ValueArray> make(const Value* begin, const Value* end);

    static Ref<ValueArray> make(std::initializer_list<Value> values) {
        auto array = ValueArray::make(values.size());

        array->copy(values.begin(), values.end());

        return array;
    }

    static Ref<ValueArray> make(std::vector<Value>&& values) {
        auto array = ValueArray::make(values.size());
        if (!values.empty()) {
            auto* dst = array->begin();
            for (size_t i = 0; i < values.size(); ++i) {
                dst[i] = std::move(values[i]);
            }
        }
        return array;
    }

private:
    size_t _size;

    friend InlineContainerAllocator<ValueArray, Value>;

    explicit ValueArray(size_t size);
};

} // namespace Valdi
