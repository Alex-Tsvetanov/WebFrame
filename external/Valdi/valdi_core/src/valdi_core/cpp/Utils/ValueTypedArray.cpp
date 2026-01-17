//
//  ValueTypedArray.cpp
//  ValdiRuntime
//
//  Created by Brandon Francis on 8/4/18.
//

#include "valdi_core/cpp/Utils/ValueTypedArray.hpp"

#include "valdi_core/cpp/Utils/StringBox.hpp"
#include "valdi_core/cpp/Utils/StringCache.hpp"

namespace Valdi {

ValueTypedArray::ValueTypedArray(TypedArrayType type, const Ref<Bytes>& buffer)
    : ValueTypedArray(type, BytesView(buffer)) {}
ValueTypedArray::ValueTypedArray(TypedArrayType type, BytesView buffer) : _type(type), _buffer(std::move(buffer)) {}

ValueTypedArray::~ValueTypedArray() = default;

TypedArrayType ValueTypedArray::getType() const {
    return _type;
}

const BytesView& ValueTypedArray::getBuffer() const {
    return _buffer;
}

bool ValueTypedArray::operator==(const ValueTypedArray& other) const {
    return _type == other._type && _buffer == other._buffer;
}

StringBox stringForTypedArrayType(const TypedArrayType& type) {
    static auto int8Array = STRING_LITERAL("Int8Array");
    static auto int16Array = STRING_LITERAL("Int16Array");
    static auto int32Array = STRING_LITERAL("Int32Array");
    static auto uint8Array = STRING_LITERAL("Uint8Array");
    static auto uint8ClampedArray = STRING_LITERAL("Uint8ClampedArray");
    static auto uint16Array = STRING_LITERAL("Uint16Array");
    static auto uint32Array = STRING_LITERAL("Uint32Array");
    static auto float32Array = STRING_LITERAL("Float32Array");
    static auto float64Array = STRING_LITERAL("Float64Array");
    static auto arrayBuffer = STRING_LITERAL("ArrayBuffer");

    switch (type) {
        case Int8Array:
            return int8Array;
        case Int16Array:
            return int16Array;
        case Int32Array:
            return int32Array;
        case Uint8Array:
            return uint8Array;
        case Uint8ClampedArray:
            return uint8ClampedArray;
        case Uint16Array:
            return int16Array;
        case Uint32Array:
            return uint32Array;
        case Float32Array:
            return float32Array;
        case Float64Array:
            return float64Array;
        case ArrayBuffer:
            return arrayBuffer;
    }
}

} // namespace Valdi
