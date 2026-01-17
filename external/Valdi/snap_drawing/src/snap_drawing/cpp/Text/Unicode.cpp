//
//  Unicode.cpp
//  snap_drawing
//
//  Created by Simon Corsin on 1/4/23.
//

#include "snap_drawing/cpp/Text/Unicode.hpp"
#include "hb.h"

namespace snap::drawing {

constexpr Character kLREChar = 0x202a;
constexpr Character kLRIChar = 0x2066;

constexpr Character kVariationSelectorStart = 0xFE00;
constexpr Character kVariationSelectorEnd = 0xFE0F;
constexpr Character kVariationSelectorSupplementStart = 0xE0100;
constexpr Character kVariationSelectorSupplementEnd = 0xE01EF;

bool Unicode::isBreakingWhitespace(Character c) {
    switch (c) {
        case 0x0020: // SPACE
        // case 0x00A0: // NO-BREAK SPACE
        case 0x1680: // OGHAM SPACE MARK
        case 0x180E: // MONGOLIAN VOWEL SEPARATOR
        case 0x2000: // EN QUAD
        case 0x2001: // EM QUAD
        case 0x2002: // EN SPACE (nut)
        case 0x2003: // EM SPACE (mutton)
        case 0x2004: // THREE-PER-EM SPACE (thick space)
        case 0x2005: // FOUR-PER-EM SPACE (mid space)
        case 0x2006: // SIX-PER-EM SPACE
        case 0x2007: // FIGURE SPACE
        case 0x2008: // PUNCTUATION SPACE
        case 0x2009: // THIN SPACE
        case 0x200A: // HAIR SPACE
        case 0x200B: // ZERO WIDTH SPACE
        case 0x202F: // NARROW NO-BREAK SPACE
        case 0x205F: // MEDIUM MATHEMATICAL SPACE
        case 0x3000: // IDEOGRAPHIC SPACE
                     // case 0xFEFF: // ZERO WIDTH NO-BREAK SPACE
            return true;
        default:
            return false;
    }
}

bool Unicode::isNewline(Character c) {
    switch (c) {
        case 0x000A:
        case 0x0D0A:
            return true;
        default:
            return false;
    }
}

bool Unicode::isUnbreakableMarker(Character c) {
    switch (c) {
        case 0x034F: // COMBINING GRAPHEME JOINER
        case 0x061C: // ARABIC LETTER MARK
        case 0x200C: // ZERO WIDTH NON-JOINER
        case 0x200D: // ZERO WIDTH JOINER
        case 0xFEFF: // BYTE-ORDER MARK
            return true;
        default:
            return isVariationSelector(c);
    }
}

inline static bool isBMPVariationSelector(Character c) {
    return c >= kVariationSelectorStart && c <= kVariationSelectorEnd;
}

inline static bool isVariationSelectorSupplement(Character c) {
    return c >= kVariationSelectorSupplementStart && c <= kVariationSelectorSupplementEnd;
}

bool Unicode::isVariationSelector(Character c) {
    return isBMPVariationSelector(c) || isVariationSelectorSupplement(c);
}

bool Unicode::isBidiControl(Character c) {
    return (c - kLREChar) < 5 || (c - kLRIChar) < 4;
}

std::string TextScript::toString() const {
    std::string out;
    out.resize(4);

    hb_tag_to_string(code, out.data());

    return out;
}

bool TextScript::isStrong() const {
    return code != HB_SCRIPT_UNKNOWN && code != HB_SCRIPT_INHERITED && code != HB_SCRIPT_COMMON;
}

TextScript TextScript::fromCharacter(Character c) {
    static auto* kUnicodeFont = hb_unicode_funcs_get_default();
    return TextScript(hb_unicode_script(kUnicodeFont, c));
}

TextScript TextScript::common() {
    return TextScript(HB_SCRIPT_COMMON);
}

static_assert(TextScript::invalid().code == HB_SCRIPT_INVALID);

} // namespace snap::drawing
