//
//  CSSAttributes.hpp
//  valdi-ios
//
//  Created by Simon Corsin on 10/31/19.
//

#pragma once

#include "valdi/runtime/Attributes/AttributeOwner.hpp"
#include "valdi_core/cpp/Utils/ValdiObject.hpp"
#include "valdi_core/cpp/Utils/Value.hpp"
#include <vector>

namespace Valdi {

struct CSSAttribute {
    AttributeId id;
    Value value;
};

struct CSSStyleDeclaration {
    CSSAttribute attribute;
    int priority = 0;
    int order = 0;
    int id = 0;
};

class CSSDocument;
class CSSAttributes : public ValdiObject, public AttributeOwner {
public:
    explicit CSSAttributes(std::vector<CSSStyleDeclaration> styles);
    ~CSSAttributes() override;

    const std::vector<CSSStyleDeclaration>& getStyles() const;

    VALDI_CLASS_HEADER(CSSAttributes)

    int getAttributePriority(AttributeId id) const override;

    StringBox getAttributeSource(AttributeId id) const override;

private:
    std::vector<CSSStyleDeclaration> _styles;
};

} // namespace Valdi
