//
//  YGFloatOptionalAttributeHandlerDelegate.cpp
//  valdi-ios
//
//  Created by Simon Corsin on 8/3/21.
//

#include "valdi/runtime/Attributes/Yoga/YGFloatOptionalAttributeHandlerDelegate.hpp"
#include "valdi/runtime/Attributes/ValueConverters.hpp"
#include "valdi_core/cpp/Attributes/AttributeUtils.hpp"

namespace Valdi {

YGFloatOptionalAttributeHandlerDelegate::YGFloatOptionalAttributeHandlerDelegate(
    YGNodeValueGetterSetter<YGFloatOptional> getterSetter)
    : YogaGetterSetterAttributeHandlerDelegate<YGFloatOptional>(getterSetter) {}

Result<Void> YGFloatOptionalAttributeHandlerDelegate::onApply(YGNodeRef node, const Value& value) {
    auto val = ValueConverter::toDouble(value);
    if (!val) {
        return val.moveError();
    }

    return setValue(node, YGFloatOptional(static_cast<float>(val.value())));
}

} // namespace Valdi
