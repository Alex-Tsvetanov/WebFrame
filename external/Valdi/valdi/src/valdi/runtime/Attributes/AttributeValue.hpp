//
//  AttributeValue.hpp
//  valdi-pc
//
//  Created by Simon Corsin on 5/15/20.
//

#pragma once

#include "valdi/runtime/Attributes/AttributeIds.hpp"
#include "valdi/runtime/Attributes/PreprocessorCache.hpp"
#include "valdi_core/cpp/Utils/Result.hpp"
#include "valdi_core/cpp/Utils/SmallVector.hpp"
#include "valdi_core/cpp/Utils/Value.hpp"

namespace Valdi {

class AttributeOwner;
class AttributeHandler;

struct AttributeValue {
    const AttributeOwner* owner;
    Value value;
    Result<PreprocessedValue> preprocessedValue;

    AttributeValue(const AttributeOwner* owner, Value&& value);

    Value dump(AttributeId attributeId) const;

    Result<Value> preprocessValue(const AttributeHandler* handler);
};

struct ResolvedAttributeValueIndex {
    size_t index;
    int priority;
};

struct AttributeValueCollection {
    SmallVector<AttributeValue, 2> values;
    size_t resolvedValueIndex = 0;

    AttributeValueCollection();

    bool updateResolvedValueIndex(AttributeId attributeId);

    ResolvedAttributeValueIndex resolveValueIndexAndPriority(AttributeId attributeId) const;
};

} // namespace Valdi
