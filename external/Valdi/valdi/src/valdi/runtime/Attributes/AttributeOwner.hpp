//
//  AttributeOwner.hpp
//  valdi-ios
//
//  Created by Simon Corsin on 9/16/19.
//

#pragma once

#include "valdi/runtime/Attributes/AttributeIds.hpp"
#include "valdi_core/cpp/Utils/StringBox.hpp"

namespace Valdi {

/**
 Attribute owner priority for an attribute which was set explicitly in native code.
 */
constexpr int kAttributeOwnerPriorityOverriden = 0;
/**
 Attribute owner priority for an attribute which was overriden from a parent ViewNode
 */
constexpr int kAttributeOwnerPriorityParentOverriden = 1;
/**
 Attribute owner priority for a ViewNode which is not root, and not a slot
 */
constexpr int kAttributeOwnerPriorityViewNode = 2;
/**
 Attribute owner priority for a CSS node managing the root node of a child tree.
 */
constexpr int kAttributeOwnerPriorityCSSParentOverriden = 3;
/**
 Attribute owner priority for a CSS node which is not root, and not within a ViewNode slot
 */
constexpr int kAttributeOwnerPriorityCSS = 4;

/**
 Attribute owner priority for a placeholder attribute value
 */
constexpr int kAttributeOwnerPriorityPlaceholder = 1000;

class AttributeOwner {
public:
    virtual ~AttributeOwner() = default;

    /**
     Returns a score how important attributes applied with this
     owner are. Lower values take priority over higher values.
     This is used to resolve conflict between multiple owners
     wanting to apply the same attribute.
     */
    virtual int getAttributePriority(AttributeId id) const = 0;

    /**
     Returns a description of where the attribute is from.
     */
    virtual StringBox getAttributeSource(AttributeId id) const = 0;

    /**
     Returns an AttributeOwner singleton which can be used to represent
     attributes applied from native code (in Objective-C or Kotlin).
     */
    static AttributeOwner* getNativeOverridenAttributeOwner();

    /**
     Returns an AttributeOwner singleton representing a placeholder attribute value
     */
    static AttributeOwner* getPlaceholderAttributeOwner();
};
} // namespace Valdi
