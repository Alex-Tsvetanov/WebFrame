//
//  CompositeAttributeUtils.cpp
//  valdi-ios
//
//  Created by Simon Corsin on 10/1/18.
//

#include "valdi/runtime/Attributes/CompositeAttributeUtils.hpp"
#include "valdi/runtime/Attributes/AttributeHandlerDelegate.hpp"

namespace Valdi {

void bindCompositeAttribute(AttributeIds& attributeIds,
                            AttributeHandlerById& attributeHandlers,
                            const Ref<CompositeAttribute>& compositeAttribute,
                            const Ref<AttributeHandlerDelegate>& delegate,
                            bool requiresView) {
    auto compositeName = attributeIds.getNameForId(compositeAttribute->getAttributeId());

    attributeHandlers[compositeAttribute->getAttributeId()] =
        AttributeHandler(compositeAttribute->getAttributeId(),
                         compositeName,
                         delegate,
                         compositeAttribute,
                         requiresView,
                         compositeAttribute->shouldInvalidateLayoutOnChange());
    for (const auto& part : compositeAttribute->getParts()) {
        auto partName = attributeIds.getNameForId(part.attributeId);
        attributeHandlers[part.attributeId] =
            AttributeHandler(part.attributeId, partName, compositeAttribute, false, part.invalidateLayoutOnChange);
    }
}

} // namespace Valdi
