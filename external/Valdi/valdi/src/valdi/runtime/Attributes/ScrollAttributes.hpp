//
//  ScrollAttributes.hpp
//  valdi
//
//  Created by Simon Corsin on 8/6/21.
//

#pragma once

#include "valdi/runtime/Attributes/AttributeHandler.hpp"
#include "valdi/runtime/Attributes/AttributeHandlerDelegateWithCallable.hpp"

namespace Valdi {

class ScrollAttributes {
public:
    explicit ScrollAttributes(AttributeIds& attributeIds);

    void bind(AttributeHandlerById& attributes);

private:
    void bindAttribute(std::string_view attribute,
                       AttributeHandlerById& attributes,
                       AttributeHandlerDelegateWithCallable::Applier applier,
                       AttributeHandlerDelegateWithCallable::Resetter resetter);

    void bindAttribute(std::string_view attribute,
                       AttributeHandlerById& attributes,
                       const Ref<AttributeHandlerDelegate>& delegate);

    AttributeIds& _attributeIds;
};

} // namespace Valdi
