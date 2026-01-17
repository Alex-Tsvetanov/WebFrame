//
//  ValueTypedArray.h
//  ValdiRuntime
//
//  Created by Brandon Francis on 8/3/18.
//

#pragma once

#include "valdi_core/cpp/Utils/Bytes.hpp"
#include <vector>

namespace Valdi {

class StringBox;

enum TypedArrayType {
    Int8Array,
    Int16Array,
    Int32Array,
    Uint8Array,
    Uint8ClampedArray,
    Uint16Array,
    Uint32Array,
    Float32Array,
    Float64Array,
    ArrayBuffer,
};

// Default type to use when converting from a buffer without an explicit type
constexpr TypedArrayType kDefaultTypedArrayType = TypedArrayType::Uint8Array;

/**
 A container for holding a TypedArray type and buffer pair.
 */
class ValueTypedArray : public SimpleRefCountable {
public:
    ValueTypedArray(TypedArrayType type, const Ref<Bytes>& buffer);
    ValueTypedArray(TypedArrayType type, BytesView buffer);
    ~ValueTypedArray() override;

    TypedArrayType getType() const;
    const BytesView& getBuffer() const;

    bool operator==(const ValueTypedArray& other) const;

private:
    TypedArrayType _type;
    BytesView _buffer;
};

StringBox stringForTypedArrayType(const TypedArrayType& type);

} // namespace Valdi
