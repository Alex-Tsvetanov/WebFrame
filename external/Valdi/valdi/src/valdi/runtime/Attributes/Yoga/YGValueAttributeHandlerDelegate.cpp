//
//  YGValueAttributeHandlerDelegate.cpp
//  valdi-ios
//
//  Created by Simon Corsin on 8/3/21.
//

#include "valdi/runtime/Attributes/Yoga/YGValueAttributeHandlerDelegate.hpp"

namespace Valdi {

YGValueAttributeHandlerDelegate::YGValueAttributeHandlerDelegate(YGNodeValueGetterSetter<YGCompactValue> getterSetter)
    : YogaGetterSetterAttributeHandlerDelegate<YGCompactValue>(getterSetter) {}

Result<Void> YGValueAttributeHandlerDelegate::onApply(YGNodeRef node, const Value& value) {
    auto ygValue = valueToYGValue(value);
    if (!ygValue) {
        return ygValue.moveError();
    }

    return setValue(node, ygValue.value());
}

} // namespace Valdi
