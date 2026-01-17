//
//  ValueUtils.hpp
//  ValdiRuntime
//
//  Created by Simon Corsin on 5/27/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

#pragma once

#include "valdi_core/cpp/Utils/ByteBuffer.hpp"
#include "valdi_core/cpp/Utils/Bytes.hpp"
#include "valdi_core/cpp/Utils/Result.hpp"
#include "valdi_core/cpp/Utils/Value.hpp"
#include "valdi_core/cpp/Utils/ValueFunction.hpp"

namespace Valdi {

class JSONWriter;
class JSONReader;

// Very basic primitive to serialize map Values into strings.
// Only supports map with no nested object currently.
Value deserializeValue(const Byte* data, size_t size);
Ref<ByteBuffer> serializeValue(const Value& value);

Ref<ByteBuffer> valueToJson(const Value& value);
std::string valueToJsonString(const Value& value);
void writeValueToJSONWriter(const Value& value, JSONWriter& writer);

Result<Value> jsonToValue(const Byte* data, size_t size);
Result<Value> jsonToValue(const std::string_view& str);

/**
 * Implementation of JSON parsing that is fully JSON compliant
 */
Result<Value> correctJsonToValue(const std::string_view& str);

/**
 * Implementation of JSON parsing that prefers performance over
 * correctness. Does not support all of the JSON specifications.
 */
Result<Value> fastJsonToValue(const std::string_view& str);

Value readValueFromJSONReader(JSONReader& reader);

} // namespace Valdi
