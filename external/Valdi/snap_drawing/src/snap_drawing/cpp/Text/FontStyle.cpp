//
//  FontStyle.cpp
//  snap_drawing-macos
//
//  Created by Simon Corsin on 1/28/22.
//

#include "snap_drawing/cpp/Text/FontStyle.hpp"
#include "valdi_core/cpp/Utils/Format.hpp"
#include "valdi_core/cpp/Utils/Result.hpp"
#include "valdi_core/cpp/Utils/StringBox.hpp"
#include "valdi_core/cpp/Utils/StringCache.hpp"

namespace snap::drawing {

static inline FontWeight fromSkiaWeight(int weight) {
    switch (weight) {
        case SkFontStyle::kInvisible_Weight:
            return FontWeightInvisible;
        case SkFontStyle::kThin_Weight:
            return FontWeightThin;
        case SkFontStyle::kExtraLight_Weight:
            return FontWeightExtraLight;
        case SkFontStyle::kLight_Weight:
            return FontWeightLight;
        case SkFontStyle::kNormal_Weight:
            return FontWeightNormal;
        case SkFontStyle::kMedium_Weight:
            return FontWeightMedium;
        case SkFontStyle::kSemiBold_Weight:
            return FontWeightSemiBold;
        case SkFontStyle::kBold_Weight:
            return FontWeightBold;
        case SkFontStyle::kExtraBold_Weight:
            return FontWeightExtraBold;
        case SkFontStyle::kBlack_Weight:
            return FontWeightBlack;
        case SkFontStyle::kExtraBlack_Weight:
            return FontWeightExtraBlack;
    }
    return FontWeightNormal;
}

static inline FontWidth fromSkiaWidth(int width) {
    switch (width) {
        case SkFontStyle::kUltraCondensed_Width:
            return FontWidthUltraCondensed;
        case SkFontStyle::kExtraCondensed_Width:
            return FontWidthExtraCondensed;
        case SkFontStyle::kCondensed_Width:
            return FontWidthCondensed;
        case SkFontStyle::kSemiCondensed_Width:
            return FontWidthSemiCondensed;
        case SkFontStyle::kNormal_Width:
            return FontWidthNormal;
        case SkFontStyle::kSemiExpanded_Width:
            return FontWidthSemiExpanded;
        case SkFontStyle::kExpanded_Width:
            return FontWidthExpanded;
        case SkFontStyle::kExtraExpanded_Width:
            return FontWidthExtraExpanded;
        case SkFontStyle::kUltraExpanded_Width:
            return FontWidthUltraExpanded;
    }
    return FontWidthNormal;
}

static inline FontSlant fromSkiaSlant(SkFontStyle::Slant slant) {
    switch (slant) {
        case SkFontStyle::kUpright_Slant:
            return FontSlantUpright;
        case SkFontStyle::kItalic_Slant:
            return FontSlantItalic;
        case SkFontStyle::kOblique_Slant:
            return FontSlantOblique;
    }
}

static inline SkFontStyle::Weight toSkiaWeight(FontWeight weight) {
    switch (weight) {
        case FontWeightInvisible:
            return SkFontStyle::kInvisible_Weight;
        case FontWeightThin:
            return SkFontStyle::kThin_Weight;
        case FontWeightExtraLight:
            return SkFontStyle::kExtraLight_Weight;
        case FontWeightLight:
            return SkFontStyle::kLight_Weight;
        case FontWeightNormal:
            return SkFontStyle::kNormal_Weight;
        case FontWeightMedium:
            return SkFontStyle::kMedium_Weight;
        case FontWeightSemiBold:
            return SkFontStyle::kSemiBold_Weight;
        case FontWeightBold:
            return SkFontStyle::kBold_Weight;
        case FontWeightExtraBold:
            return SkFontStyle::kExtraBold_Weight;
        case FontWeightBlack:
            return SkFontStyle::kBlack_Weight;
        case FontWeightExtraBlack:
            return SkFontStyle::kExtraBlack_Weight;
    }
}

static inline SkFontStyle::Width toSkiaWidth(FontWidth width) {
    switch (width) {
        case FontWidthUltraCondensed:
            return SkFontStyle::kUltraCondensed_Width;
        case FontWidthExtraCondensed:
            return SkFontStyle::kExtraCondensed_Width;
        case FontWidthCondensed:
            return SkFontStyle::kCondensed_Width;
        case FontWidthSemiCondensed:
            return SkFontStyle::kSemiCondensed_Width;
        case FontWidthNormal:
            return SkFontStyle::kNormal_Width;
        case FontWidthSemiExpanded:
            return SkFontStyle::kSemiExpanded_Width;
        case FontWidthExpanded:
            return SkFontStyle::kExpanded_Width;
        case FontWidthExtraExpanded:
            return SkFontStyle::kExtraExpanded_Width;
        case FontWidthUltraExpanded:
            return SkFontStyle::kUltraExpanded_Width;
    }
}

static inline SkFontStyle::Slant toSkiaSlant(FontSlant slant) {
    switch (slant) {
        case FontSlantUpright:
            return SkFontStyle::kUpright_Slant;
        case FontSlantItalic:
            return SkFontStyle::kItalic_Slant;
        case FontSlantOblique:
            return SkFontStyle::kOblique_Slant;
    }
}

FontStyle::FontStyle(FontWidth width, FontWeight weight, FontSlant slant)
    : _width(width), _weight(weight), _slant(slant) {}

FontStyle::FontStyle(SkFontStyle skFontStyle)
    : _width(fromSkiaWidth(skFontStyle.width())),
      _weight(fromSkiaWeight(skFontStyle.weight())),
      _slant(fromSkiaSlant(skFontStyle.slant())) {}

FontWidth FontStyle::getWidth() const {
    return _width;
}

void FontStyle::setWidth(FontWidth width) {
    _width = width;
}

FontWeight FontStyle::getWeight() const {
    return _weight;
}

void FontStyle::setWeight(FontWeight weight) {
    _weight = weight;
}

Valdi::StringBox FontStyle::fontWeightToString(FontWeight fontWeight) {
    static auto kWeightInvisible = STRING_LITERAL("invisible");
    static auto kWeightThin = STRING_LITERAL("thin");
    static auto kWeightExtraLight = STRING_LITERAL("extralight");
    static auto kWeightLight = STRING_LITERAL("light");
    static auto kWeightNormal = STRING_LITERAL("normal");
    static auto kWeightMedium = STRING_LITERAL("medium");
    static auto kWeightSemiBold = STRING_LITERAL("semibold");
    static auto kWeightBold = STRING_LITERAL("bold");
    static auto kWeightExtraBold = STRING_LITERAL("extrabold");
    static auto kWeightBlack = STRING_LITERAL("black");
    static auto kWeightExtraBlack = STRING_LITERAL("extrablack");

    switch (fontWeight) {
        case FontWeightInvisible:
            return kWeightInvisible;
        case FontWeightThin:
            return kWeightThin;
        case FontWeightExtraLight:
            return kWeightExtraLight;
        case FontWeightLight:
            return kWeightLight;
        case FontWeightNormal:
            return kWeightNormal;
        case FontWeightMedium:
            return kWeightMedium;
        case FontWeightSemiBold:
            return kWeightSemiBold;
        case FontWeightBold:
            return kWeightBold;
        case FontWeightExtraBold:
            return kWeightExtraBold;
        case FontWeightBlack:
            return kWeightBlack;
        case FontWeightExtraBlack:
            return kWeightExtraBlack;
    }
}

Valdi::Result<std::pair<Valdi::StringBox, FontWeight>> FontStyle::getFamilyNameAndFontWeight(
    const Valdi::StringBox& fontName) {
    auto fontKey = fontName;
    auto fontWeight = FontWeightNormal;
    auto separatorIndex = fontName.lastIndexOf('-');
    if (separatorIndex) {
        auto components = fontName.split(*separatorIndex);
        fontKey = components.first;

        auto fontWeightResult = parseWeight(components.second.toStringView());
        if (!fontWeightResult) {
            return fontWeightResult.moveError();
        }
        fontWeight = fontWeightResult.value();
    }
    return std::pair<Valdi::StringBox, FontWeight>(fontKey, fontWeight);
}

Valdi::Result<FontWeight> FontStyle::parseWeight(std::string_view weightStr) {
    if (weightStr == "light") {
        return FontWeightLight;
    } else if (weightStr == "regular" || weightStr == "normal") {
        return FontWeightNormal;
    } else if (weightStr == "medium") {
        return FontWeightMedium;
    } else if (weightStr == "semibold" || weightStr == "demibold" || weightStr == "demi-bold") {
        return FontWeightSemiBold;
    } else if (weightStr == "bold") {
        return FontWeightBold;
    } else if (weightStr == "black") {
        return FontWeightBlack;
    } else {
        return Valdi::Error(STRING_FORMAT("Could not resolve weight '{}'", weightStr));
    }
}

Valdi::Result<FontSlant> FontStyle::parseSlant(std::string_view slantStr) {
    if (slantStr == "normal" || slantStr == "upright") {
        return FontSlantUpright;
    } else if (slantStr == "italic") {
        return FontSlantItalic;
    } else if (slantStr == "oblique") {
        return FontSlantOblique;
    } else {
        return Valdi::Error(STRING_FORMAT("Could not resolve slant '{}'", slantStr));
    }
}

FontSlant FontStyle::getSlant() const {
    return _slant;
}

void FontStyle::setSlant(FontSlant slant) {
    _slant = slant;
}

size_t FontStyle::hash() const {
    return static_cast<size_t>(_width) | static_cast<size_t>(_weight) << 8 | static_cast<size_t>(_slant) << 16;
}

bool FontStyle::operator==(const FontStyle& other) const {
    return _width == other._width && _weight == other._weight && _slant == other._slant;
}

bool FontStyle::operator!=(const FontStyle& other) const {
    return !(*this == other);
}

SkFontStyle FontStyle::getSkValue() const {
    return SkFontStyle(toSkiaWeight(_weight), toSkiaWidth(_width), toSkiaSlant(_slant));
}

} // namespace snap::drawing

namespace std {

std::size_t hash<snap::drawing::FontStyle>::operator()(const snap::drawing::FontStyle& k) const noexcept {
    return k.hash();
}

} // namespace std
