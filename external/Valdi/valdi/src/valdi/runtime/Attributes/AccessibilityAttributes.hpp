#pragma once

#include "valdi/runtime/Attributes/AttributeHandler.hpp"
#include "valdi/runtime/Attributes/AttributeHandlerDelegateWithCallable.hpp"

#include "valdi/runtime/Utils/SharedContainers.hpp"

namespace Valdi {

/**
 * Provide capabilities to bind all the accessibility attributes:
 * - accessibilityCategory
 * - accessibilityNavigation
 * - accessibilityPriority
 * - accessibilityLabel
 * - accessibilityHint
 * - accessibilityValue
 * - accessibilityId
 * - accessibilityStateDisabled
 * - accessibilityStateSelected
 * - accessibilityStateLiveRegion
 */
class AccessibilityAttributes : public SimpleRefCountable {
public:
    explicit AccessibilityAttributes(AttributeIds& attributeIds);
    ~AccessibilityAttributes() override;

    void bind(AttributeHandlerById& attributes);

private:
    AttributeIds& _attributeIds;

    Shared<FlatMap<StringBox, int>> _accessibilityCategoryMap;
    Shared<FlatMap<StringBox, int>> _accessibilityNavigationMap;
};

} // namespace Valdi
