//
//  AttributedText.cpp
//  valdi-skia
//
//  Created by Simon Corsin on 6/30/20.
//

#include "snap_drawing/cpp/Text/AttributedText.hpp"

namespace snap::drawing {

AttributedText::AttributedText(AttributedText::Parts parts)
    : Valdi::TextAttributeValueBase<AttributedTextPartStyle>(std::move(parts)) {}

AttributedText::~AttributedText() = default;

} // namespace snap::drawing
