//
//  CompositeAttributeInfo.cpp
//  ValdiRuntimeIOS
//
//  Created by Simon Corsin on 7/9/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

#include "valdi/runtime/Attributes/CompositeAttribute.hpp"

namespace Valdi {

CompositeAttributePart::CompositeAttributePart(AttributeId attributeId, bool optional, bool invalidateLayoutOnChange)
    : attributeId(attributeId), optional(optional), invalidateLayoutOnChange(invalidateLayoutOnChange) {}

CompositeAttribute::CompositeAttribute(AttributeId attributeId,
                                       const StringBox& name,
                                       std::vector<CompositeAttributePart> parts)
    : _attributeId(attributeId), _name(name), _parts(std::move(parts)) {
    for (const auto& part : _parts) {
        if (part.invalidateLayoutOnChange) {
            _invalidateLayoutOnChange = true;
            break;
        }
    }
}

CompositeAttribute::~CompositeAttribute() = default;

AttributeId CompositeAttribute::getAttributeId() const {
    return _attributeId;
}

const StringBox& CompositeAttribute::getName() const {
    return _name;
}

const std::vector<CompositeAttributePart>& CompositeAttribute::getParts() const {
    return _parts;
}

} // namespace Valdi
