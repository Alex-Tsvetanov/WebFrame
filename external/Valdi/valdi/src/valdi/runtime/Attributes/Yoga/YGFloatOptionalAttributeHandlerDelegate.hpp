//
//  YGFloatOptionalAttributeHandlerDelegate.hpp
//  valdi-ios
//
//  Created by Simon Corsin on 8/3/21.
//

#pragma once

#include "valdi/runtime/Attributes/Yoga/YogaGetterSetterAttributeHandlerDelegate.hpp"

namespace Valdi {

class YGFloatOptionalAttributeHandlerDelegate : public YogaGetterSetterAttributeHandlerDelegate<YGFloatOptional> {
public:
    explicit YGFloatOptionalAttributeHandlerDelegate(YGNodeValueGetterSetter<YGFloatOptional> getterSetter);

protected:
    Result<Void> onApply(YGNodeRef node, const Value& value) override;
};

} // namespace Valdi
