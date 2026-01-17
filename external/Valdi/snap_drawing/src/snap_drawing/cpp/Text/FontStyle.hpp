//
//  FontStyle.hpp
//  snap_drawing-macos
//
//  Created by Simon Corsin on 1/28/22.
//

#pragma once

#include "include/core/SkFontStyle.h"
#include "valdi_core/cpp/Utils/Result.hpp"
#include "valdi_core/cpp/Utils/StringBox.hpp"
#include <functional>

namespace snap::drawing {

enum FontWidth : uint8_t {
    FontWidthUltraCondensed,
    FontWidthExtraCondensed,
    FontWidthCondensed,
    FontWidthSemiCondensed,
    FontWidthNormal,
    FontWidthSemiExpanded,
    FontWidthExpanded,
    FontWidthExtraExpanded,
    FontWidthUltraExpanded,
};

enum FontWeight : uint8_t {
    FontWeightInvisible,
    FontWeightThin,
    FontWeightExtraLight,
    FontWeightLight,
    FontWeightNormal,
    FontWeightMedium,
    FontWeightSemiBold,
    FontWeightBold,
    FontWeightExtraBold,
    FontWeightBlack,
    FontWeightExtraBlack,
};

enum FontSlant : uint8_t {
    FontSlantUpright,
    FontSlantItalic,
    FontSlantOblique,
};

class FontStyle {
public:
    FontStyle(FontWidth width, FontWeight weight, FontSlant slant);
    explicit FontStyle(SkFontStyle skFontStyle);

    FontWidth getWidth() const;
    void setWidth(FontWidth width);

    FontWeight getWeight() const;
    void setWeight(FontWeight weight);

    static Valdi::StringBox fontWeightToString(FontWeight fontWeight);

    static Valdi::Result<std::pair<Valdi::StringBox, FontWeight>> getFamilyNameAndFontWeight(
        const Valdi::StringBox& fontName);

    FontSlant getSlant() const;
    void setSlant(FontSlant slant);

    SkFontStyle getSkValue() const;

    size_t hash() const;

    bool operator==(const FontStyle& other) const;
    bool operator!=(const FontStyle& other) const;

    static Valdi::Result<FontWeight> parseWeight(std::string_view weightStr);
    static Valdi::Result<FontSlant> parseSlant(std::string_view slantStr);

private:
    FontWidth _width;
    FontWeight _weight;
    FontSlant _slant;
};

} // namespace snap::drawing

namespace std {

template<>
struct hash<snap::drawing::FontStyle> {
    std::size_t operator()(const snap::drawing::FontStyle& k) const noexcept;
};

} // namespace std
