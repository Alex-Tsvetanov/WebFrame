#include "valdi/runtime/Attributes/AccessibilityAttributes.hpp"

#include "valdi/runtime/Attributes/AttributesBinderHelper.hpp"
#include "valdi/runtime/Context/ViewNode.hpp"

namespace Valdi {

AccessibilityAttributes::AccessibilityAttributes(AttributeIds& attributeIds) : _attributeIds(attributeIds) {
    FlatMap<StringBox, int> accessibilityCategoryMap(12);
    FlatMap<StringBox, int> accessibilityNavigationMap(6);

    accessibilityCategoryMap[STRING_LITERAL("auto")] = AccessibilityCategoryAuto;
    accessibilityCategoryMap[STRING_LITERAL("view")] = AccessibilityCategoryView;
    accessibilityCategoryMap[STRING_LITERAL("image")] = AccessibilityCategoryImage;
    accessibilityCategoryMap[STRING_LITERAL("button")] = AccessibilityCategoryButton;
    accessibilityCategoryMap[STRING_LITERAL("image-button")] = AccessibilityCategoryImageButton;
    accessibilityCategoryMap[STRING_LITERAL("text")] = AccessibilityCategoryText;
    accessibilityCategoryMap[STRING_LITERAL("input")] = AccessibilityCategoryInput;
    accessibilityCategoryMap[STRING_LITERAL("link")] = AccessibilityCategoryLink;
    accessibilityCategoryMap[STRING_LITERAL("header")] = AccessibilityCategoryHeader;
    accessibilityCategoryMap[STRING_LITERAL("checkbox")] = AccessibilityCategoryCheckBox;
    accessibilityCategoryMap[STRING_LITERAL("radio")] = AccessibilityCategoryRadio;
    accessibilityCategoryMap[STRING_LITERAL("keyboard-key")] = AccessibilityCategoryKeyboardKey;

    accessibilityNavigationMap[STRING_LITERAL("auto")] = AccessibilityNavigationAuto;
    accessibilityNavigationMap[STRING_LITERAL("passthrough")] = AccessibilityNavigationPassthrough;
    accessibilityNavigationMap[STRING_LITERAL("leaf")] = AccessibilityNavigationLeaf;
    accessibilityNavigationMap[STRING_LITERAL("cover")] = AccessibilityNavigationCover;
    accessibilityNavigationMap[STRING_LITERAL("group")] = AccessibilityNavigationGroup;
    accessibilityNavigationMap[STRING_LITERAL("ignored")] = AccessibilityNavigationIgnored;

    _accessibilityCategoryMap = makeShared<FlatMap<StringBox, int>>(std::move(accessibilityCategoryMap));
    _accessibilityNavigationMap = makeShared<FlatMap<StringBox, int>>(std::move(accessibilityNavigationMap));
}

AccessibilityAttributes::~AccessibilityAttributes() = default;

void AccessibilityAttributes::bind(AttributeHandlerById& attributes) {
    AttributesBinderHelper binder(_attributeIds, attributes);

    binder.bindViewNodeEnum("accessibilityCategory",
                            &ViewNode::setAccessibilityCategory,
                            _accessibilityCategoryMap,
                            AccessibilityCategoryAuto);

    binder.bindViewNodeEnum("accessibilityNavigation",
                            &ViewNode::setAccessibilityNavigation,
                            _accessibilityNavigationMap,
                            AccessibilityNavigationAuto);

    binder.bindViewNodeFloat("accessibilityPriority", &ViewNode::setAccessibilityPriority);

    binder.bindViewNodeString("accessibilityLabel", &ViewNode::setAccessibilityLabel);
    binder.bindViewNodeString("accessibilityHint", &ViewNode::setAccessibilityHint);
    binder.bindViewNodeString("accessibilityValue", &ViewNode::setAccessibilityValue);

    binder.bindViewNodeBoolean("accessibilityStateDisabled", &ViewNode::setAccessibilityStateDisabled);
    binder.bindViewNodeBoolean("accessibilityStateSelected", &ViewNode::setAccessibilityStateSelected);
    binder.bindViewNodeBoolean("accessibilityStateLiveRegion", &ViewNode::setAccessibilityStateLiveRegion);
}

} // namespace Valdi
