//
//  AttributeValue.cpp
//  valdi-pc
//
//  Created by Simon Corsin on 5/15/20.
//

#include "valdi/runtime/Attributes/AttributeValue.hpp"
#include "valdi/runtime/Attributes/AttributeHandler.hpp"
#include "valdi/runtime/Attributes/AttributeOwner.hpp"
#include "valdi_core/cpp/Utils/StringCache.hpp"
#include "valdi_core/cpp/Utils/ValueMap.hpp"

#include <limits>

namespace Valdi {

AttributeValue::AttributeValue(const AttributeOwner* owner, Value&& value) : owner(owner), value(std::move(value)) {}

AttributeValueCollection::AttributeValueCollection() : resolvedValueIndex(std::numeric_limits<size_t>::max()) {}

bool AttributeValueCollection::updateResolvedValueIndex(AttributeId attributeId) {
    if (values.empty()) {
        resolvedValueIndex = std::numeric_limits<size_t>::max();
        return true;
    }

    auto index = resolveValueIndexAndPriority(attributeId).index;
    if (index != resolvedValueIndex) {
        resolvedValueIndex = index;
        return true;
    }

    return false;
}

ResolvedAttributeValueIndex AttributeValueCollection::resolveValueIndexAndPriority(AttributeId attributeId) const {
    int lowestPriority = std::numeric_limits<int>::max();
    size_t lowestScoreIndex = 0;
    size_t i = 0;
    for (const auto& value : values) {
        auto priority = value.owner->getAttributePriority(attributeId);
        if (priority < lowestPriority) {
            lowestPriority = priority;
            lowestScoreIndex = i;
        }
        i++;
    }

    return (ResolvedAttributeValueIndex){.index = lowestScoreIndex, .priority = lowestPriority};
}

Result<Value> AttributeValue::preprocessValue(const AttributeHandler* handler) {
    if (preprocessedValue.empty()) {
        // Value has not been preprocessed yet, do it now.
        preprocessedValue = handler->preprocess(value);
    }

    if (!preprocessedValue) {
        return preprocessedValue.error();
    }

    return preprocessedValue.value().value;
}

Value AttributeValue::dump(AttributeId attributeId) const {
    static auto kValueKey = STRING_LITERAL("value");
    static auto kSourceKey = STRING_LITERAL("source");

    auto valueMap = makeShared<ValueMap>();
    valueMap->reserve(2);
    (*valueMap)[kValueKey] = value;
    (*valueMap)[kSourceKey] = Value(owner->getAttributeSource(attributeId));

    return Value(valueMap);
}

} // namespace Valdi
