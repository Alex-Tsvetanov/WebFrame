//
//  TextAttributeValue.cpp
//  valdi_core-ios
//
//  Created by Simon Corsin on 12/20/22.
//

#include "valdi_core/cpp/Attributes/TextAttributeValue.hpp"

namespace Valdi {

TextAttributeValue::TextAttributeValue(TextAttributeValueBase<TextAttributeValueStyle>::Parts parts)
    : TextAttributeValueBase<TextAttributeValueStyle>(std::move(parts)) {}
TextAttributeValue::~TextAttributeValue() = default;

std::string TextAttributeValue::toString() const {
    std::string out;

    size_t length = getPartsSize();
    for (size_t i = 0; i < length; i++) {
        out += getContentAtIndex(i).toStringView();
    }

    return out;
}

VALDI_CLASS_IMPL(TextAttributeValue)

} // namespace Valdi
