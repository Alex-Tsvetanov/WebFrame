//
//  DjinniUtils.hpp
//  valdi-ios
//
//  Created by Simon Corsin on 16/06/22.
//

#pragma once

#include "djinni/cpp/DataRef.hpp"
#include "djinni/cpp/expected.hpp"
#include "valdi_core/cpp/Utils/Bytes.hpp"
#include "valdi_core/cpp/Utils/Shared.hpp"
#include "valdi_core/cpp/Utils/ValdiObject.hpp"
#include "valdi_core/cpp/Utils/Value.hpp"

namespace Valdi {

template<typename T>
Valdi::Shared<T> djinniObjectFromValue(const Value& value);

Valdi::BytesView bytesViewFromDataRef(const djinni::DataRef& dataRef);

djinni::DataRef dataRefFromBytesView(const Valdi::BytesView& bytesView);

struct VectorOfValues : ValdiObject {
    std::vector<Value> entries;
    VALDI_CLASS_HEADER(VectorOfValues);
};

using ES6Map = VectorOfValues;
using ES6Set = VectorOfValues;

struct ValdiOutcome : ValdiObject {
    Value result = Value::undefined();
    Value error = Value::undefined();
    VALDI_CLASS_HEADER(ValdiOutcome);
};

} // namespace Valdi
