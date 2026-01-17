//
//  CompositeAttribute.hpp
//  ValdiRuntimeIOS
//
//  Created by Simon Corsin on 7/9/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

#pragma once

#include "valdi/runtime/Attributes/AttributeIds.hpp"
#include "valdi_core/cpp/Utils/Shared.hpp"
#include "valdi_core/cpp/Utils/StringBox.hpp"
#include <vector>

namespace Valdi {

struct CompositeAttributePart {
    AttributeId attributeId;
    bool optional;
    bool invalidateLayoutOnChange;

    CompositeAttributePart(AttributeId attributeId, bool optional, bool invalidateLayoutOnChange = false);
};

class CompositeAttribute : public SimpleRefCountable {
public:
    CompositeAttribute(AttributeId attributeId, const StringBox& name, std::vector<CompositeAttributePart> parts);
    ~CompositeAttribute() override;

    AttributeId getAttributeId() const;
    const StringBox& getName() const;
    const std::vector<CompositeAttributePart>& getParts() const;

    constexpr bool shouldInvalidateLayoutOnChange() const {
        return _invalidateLayoutOnChange;
    }

private:
    AttributeId _attributeId;
    StringBox _name;
    std::vector<CompositeAttributePart> _parts;
    bool _invalidateLayoutOnChange = false;
};

} // namespace Valdi
