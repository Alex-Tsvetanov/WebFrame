//
//  CSSAttributes.cpp
//  valdi-ios
//
//  Created by Simon Corsin on 10/31/19.
//

#include "valdi/runtime/CSS/CSSAttributes.hpp"

namespace Valdi {

CSSAttributes::CSSAttributes(std::vector<CSSStyleDeclaration> styles) : _styles(std::move(styles)) {}

CSSAttributes::~CSSAttributes() = default;

const std::vector<CSSStyleDeclaration>& CSSAttributes::getStyles() const {
    return _styles;
}

int CSSAttributes::getAttributePriority(AttributeId /*id*/) const {
    return kAttributeOwnerPriorityCSS;
}

StringBox CSSAttributes::getAttributeSource(AttributeId /*id*/) const {
    static auto kSource = STRING_LITERAL("css");

    return kSource;
}

VALDI_CLASS_IMPL(CSSAttributes)

} // namespace Valdi
