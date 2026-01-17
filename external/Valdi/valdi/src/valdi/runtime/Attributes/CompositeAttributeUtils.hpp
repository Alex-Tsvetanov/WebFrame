//
//  CompositeAttributeUtils.hpp
//  valdi-ios
//
//  Created by Simon Corsin on 10/1/18.
//

#pragma once

#include "valdi/runtime/Attributes/AttributeHandler.hpp"
#include "valdi/runtime/Attributes/CompositeAttribute.hpp"
#include <memory>

namespace Valdi {

void bindCompositeAttribute(AttributeIds& attributeIds,
                            AttributeHandlerById& attributeHandlers,
                            const Ref<CompositeAttribute>& compositeAttribute,
                            const Ref<AttributeHandlerDelegate>& delegate,
                            bool requiresView);
}
