#pragma once

#include "valdi_core/cpp/Utils/Bytes.hpp"
#include "valdi_core/cpp/Utils/Value.hpp"
#include "valdi_core/cpp/Utils/ValueTypedArray.hpp"

namespace Valdi {

class ValueSchema;

struct BytesTranslator {
    using CppType = BytesView;
    using ValdiType = Value;

    static CppType toCpp(ValdiType data) {
        const auto* typedArray = data.getTypedArray();
        if (typedArray == nullptr) {
            return BytesView(nullptr, nullptr, 0);
        } else {
            return BytesView(typedArray->getBuffer());
        }
    }

    static ValdiType fromCpp(const CppType& c) {
        return Value(makeShared<ValueTypedArray>(TypedArrayType::Uint8Array, c));
    }

    using Boxed = BytesTranslator;

    static const Valdi::ValueSchema& schema();
};

} // namespace Valdi
