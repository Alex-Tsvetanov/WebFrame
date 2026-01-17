//
//  Unicode.hpp
//  snap_drawing
//
//  Created by Simon Corsin on 1/4/23.
//

#pragma once

#include "snap_drawing/cpp/Text/Character.hpp"
#include <string>

namespace snap::drawing {

/**
 Utility functions to deal with Unicode points.
 Could be later migrated to use libicu
 */
class Unicode {
public:
    /**
     Returns whether the Character represents a whitespace that can be used as word break.
     */
    static bool isBreakingWhitespace(Character c);

    /**
     Returns whether the character represents a newline
     */
    static bool isNewline(Character c);

    /**
     Returns whether the character represents a Unicode marker that
     should be treated alongside surrounding characters, meaning that
     it would be illegal to break the string on this character.
     */
    static bool isUnbreakableMarker(Character c);

    /**
     Returns whether the character represents a variation selector.
     */
    static bool isVariationSelector(Character c);

    /**
     Returns whether the character represents a bidi control character
     */
    static bool isBidiControl(Character c);
};

/**
 TextScript represents a collection of letters and signs within a writing
 systems, like Latin for example which is used in English, French, Italian...,
 or Arabic which is used in Persian, Punjabi, Sindhi etc...
 */
struct TextScript {
    uint32_t code = 0;

    constexpr TextScript() = default;
    explicit constexpr TextScript(uint32_t code) : code(code) {}

    std::string toString() const;

    constexpr bool operator==(const TextScript& other) const {
        return code == other.code;
    }

    constexpr bool operator!=(const TextScript& other) const {
        return code != other.code;
    }

    /**
     Returns whether the script is a known and not common script,
     like Latin or Arabic. Script representing punctation letters
     or whitespaces are considered weak.
     */
    bool isStrong() const;

    static TextScript fromCharacter(Character c);

    static constexpr TextScript invalid() {
        return TextScript(0);
    }

    static TextScript common();
};

} // namespace snap::drawing
