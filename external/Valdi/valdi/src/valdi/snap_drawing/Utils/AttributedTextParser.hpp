//
//  AttributedTextParser.hpp
//  valdi-desktop-apple
//
//  Created by Simon Corsin on 1/12/22.
//

#pragma once

#include "valdi_core/cpp/Utils/Result.hpp"
#include "valdi_core/cpp/Utils/ValueArray.hpp"

#include "snap_drawing/cpp/Text/AttributedText.hpp"

namespace snap::drawing {

class FontManager;

class AttributedTextParser {
public:
    static Valdi::Result<Valdi::Ref<AttributedText>> parse(FontManager& fontManager, const Valdi::Value& value);
};

} // namespace snap::drawing
