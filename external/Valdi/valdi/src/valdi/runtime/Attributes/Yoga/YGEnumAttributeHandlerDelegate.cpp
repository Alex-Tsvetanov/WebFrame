//
//  YGEnumAttributeHandlerDelegate.cpp
//  valdi-ios
//
//  Created by Simon Corsin on 8/3/21.
//

#include "valdi/runtime/Attributes/Yoga/YGEnumAttributeHandlerDelegate.hpp"
#include "valdi_core/cpp/Utils/Format.hpp"
#include "valdi_core/cpp/Utils/StringCache.hpp"

namespace Valdi {

YGEnumAttributeHandlerDelegate::YGEnumAttributeHandlerDelegate(const FlatMap<StringBox, int>& associateMap,
                                                               YGNodeValueGetterSetter<int> getterSetter)
    : YogaGetterSetterAttributeHandlerDelegate<int>(getterSetter), _associateMap(associateMap) {}

Result<Void> YGEnumAttributeHandlerDelegate::onApply(YGNodeRef node, const Value& value) {
    auto enumValueStr = value.toStringBox();

    const auto& it = _associateMap.find(enumValueStr);
    if (it == _associateMap.end()) {
        auto acceptableValues = ValueArray::make(_associateMap.size());

        size_t i = 0;
        for (const auto& valueIt : _associateMap) {
            (*acceptableValues)[i++] = Value(valueIt.first);
        }

        return Error(STRING_FORMAT(
            "Invalid value '{}', acceptable values are '{}'", enumValueStr, Value(acceptableValues).toString()));
    }

    return setValue(node, it->second);
}

} // namespace Valdi
