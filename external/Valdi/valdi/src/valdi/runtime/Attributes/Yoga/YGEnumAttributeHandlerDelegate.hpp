//
//  YGEnumAttributeHandlerDelegate.hpp
//  valdi-ios
//
//  Created by Simon Corsin on 8/3/21.
//

#pragma once

#include "valdi/runtime/Attributes/Yoga/YogaGetterSetterAttributeHandlerDelegate.hpp"
#include "valdi_core/cpp/Utils/FlatMap.hpp"
#include "valdi_core/cpp/Utils/StringBox.hpp"

namespace Valdi {

class YGEnumAttributeHandlerDelegate : public YogaGetterSetterAttributeHandlerDelegate<int> {
public:
    YGEnumAttributeHandlerDelegate(const FlatMap<StringBox, int>& associateMap,
                                   YGNodeValueGetterSetter<int> getterSetter);

protected:
    Result<Void> onApply(YGNodeRef node, const Value& value) override;

private:
    const FlatMap<StringBox, int>& _associateMap;
};

} // namespace Valdi
