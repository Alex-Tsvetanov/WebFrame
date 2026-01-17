//
//  AttributeOwner.cpp
//  valdi-ios
//
//  Created by Simon Corsin on 9/16/19.
//

#include "valdi/runtime/Attributes/AttributeOwner.hpp"
#include "valdi_core/cpp/Utils/StringCache.hpp"

namespace Valdi {

class NativeOverrideAttributeOwner : public AttributeOwner {
public:
    int getAttributePriority(AttributeId /*id*/) const override {
        return kAttributeOwnerPriorityOverriden;
    }

    StringBox getAttributeSource(AttributeId /*id*/) const override {
        static auto kSource = STRING_LITERAL("overidden");

        return kSource;
    }
};

class PlaceholderAttributeOwner : public AttributeOwner {
public:
    int getAttributePriority(AttributeId /*id*/) const override {
        return kAttributeOwnerPriorityPlaceholder;
    }

    StringBox getAttributeSource(AttributeId /*id*/) const override {
        static auto kSource = STRING_LITERAL("placeholder");

        return kSource;
    }
};

AttributeOwner* AttributeOwner::getNativeOverridenAttributeOwner() {
    static auto* kNativeOwner = new NativeOverrideAttributeOwner();
    return kNativeOwner;
}

AttributeOwner* AttributeOwner::getPlaceholderAttributeOwner() {
    static auto* kPlaceholderOwner = new PlaceholderAttributeOwner();
    return kPlaceholderOwner;
}

} // namespace Valdi
