//
//  DefaultAttributesBase.hpp
//  valdi
//
//  Created by Simon Corsin on 8/6/21.
//

#pragma once

#include "valdi/runtime/Attributes/AttributeHandler.hpp"
#include "valdi/runtime/Attributes/AttributeHandlerDelegate.hpp"
#include "valdi/runtime/Attributes/AttributeHandlerDelegateWithCallable.hpp"
#include "valdi/runtime/Attributes/CompositeAttribute.hpp"
#include "valdi/runtime/Attributes/CompositeAttributeUtils.hpp"
#include "valdi/runtime/Utils/SharedContainers.hpp"
#include "valdi_core/cpp/Utils/FlatMap.hpp"
#include "valdi_core/cpp/Utils/Format.hpp"
#include "valdi_core/cpp/Utils/StringBox.hpp"
#include "valdi_core/cpp/Utils/StringCache.hpp"
#include "valdi_core/cpp/Utils/Value.hpp"
#include "valdi_core/cpp/Utils/ValueFunction.hpp"

namespace Valdi {

class ViewTransactionScope;
class AttributeIds;

template<typename T>
using ViewNodeSetter = void (ViewNode::*)(T);

template<typename T>
using ViewNodeSetterWithViewTransactionScope = void (ViewNode::*)(ViewTransactionScope&, T);

class AttributesBinderHelper {
public:
    AttributesBinderHelper(AttributeIds& attributeIds, AttributeHandlerById& attributes);

    void bindViewNodeCallback(std::string_view attribute, ViewNodeSetter<Ref<ValueFunction>> setter);
    void bindViewNodeCallback(std::string_view attribute,
                              ViewNodeSetterWithViewTransactionScope<Ref<ValueFunction>> setter);
    void bindViewNodeBoolean(std::string_view attribute, ViewNodeSetter<bool> setter);
    void bindViewNodeBoolean(std::string_view attribute, ViewNodeSetterWithViewTransactionScope<bool> setter);
    void bindViewNodeString(std::string_view attribute, ViewNodeSetter<const StringBox&> setter);
    void bindViewNodeString(std::string_view attribute,
                            ViewNodeSetterWithViewTransactionScope<const StringBox&> setter);
    void bindViewNodeFloat(std::string_view attribute, ViewNodeSetter<float> setter);
    void bindViewNodeFloat(std::string_view attribute, ViewNodeSetterWithViewTransactionScope<float> setter);
    void bindViewNodeInt(std::string_view attribute, ViewNodeSetter<int> setter);
    void bindViewNodeInt(std::string_view attribute, ViewNodeSetterWithViewTransactionScope<int> setter);
    void bindViewNodeEnum(std::string_view attribute,
                          ViewNodeSetter<int> setter,
                          const Shared<FlatMap<StringBox, int>>& map,
                          int defaultValue);

    void bind(std::string_view attribute,
              AttributeHandlerDelegateWithCallable::Applier applier,
              AttributeHandlerDelegateWithCallable::Resetter resetter);

    void bind(std::string_view attribute, const Ref<AttributeHandlerDelegate>& delegate);

    void bindComposite(std::string_view attribute,
                       std::vector<CompositeAttributePart>&& compositeParts,
                       AttributeHandlerDelegateWithCallable::Applier applier,
                       AttributeHandlerDelegateWithCallable::Resetter resetter);

private:
    AttributeIds& _attributeIds;
    AttributeHandlerById& _attributes;
};

} // namespace Valdi
