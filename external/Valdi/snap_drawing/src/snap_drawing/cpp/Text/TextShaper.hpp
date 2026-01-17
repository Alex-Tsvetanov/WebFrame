//
//  TextShaper.hpp
//  snap_drawing-macos
//
//  Created by Simon Corsin on 3/8/22.
//

#pragma once

#include "snap_drawing/cpp/Text/Character.hpp"
#include "snap_drawing/cpp/Text/Font.hpp"
#include "snap_drawing/cpp/Text/Unicode.hpp"
#include "snap_drawing/cpp/Utils/Aliases.hpp"
#include "valdi_core/cpp/Utils/SmallVector.hpp"

namespace snap::drawing {

constexpr uint32_t kUnsafeToBreakMask = 1 << 31;
constexpr uint32_t kCharacterMask = ~kUnsafeToBreakMask;

struct ShapedGlyph {
    Scalar offsetX;
    Scalar offsetY;
    Scalar advanceX;
    SkGlyphID glyphID;

public:
    constexpr Character character() const {
        return characterAndFlags & kCharacterMask;
    }

    constexpr void setCharacter(Character character, bool unsafeToBreak) {
        characterAndFlags = (character & kCharacterMask) | (unsafeToBreak ? kUnsafeToBreakMask : 0);
    }

    constexpr bool unsafeToBreak() const {
        return (characterAndFlags & kUnsafeToBreakMask) != 0;
    }

    constexpr bool operator==(const ShapedGlyph& other) const {
        return offsetX == other.offsetX && offsetY == other.offsetY && advanceX == other.advanceX &&
               glyphID == other.glyphID && characterAndFlags == other.characterAndFlags;
    }

    constexpr bool operator!=(const ShapedGlyph& other) const {
        return !(*this == other);
    }

private:
    uint32_t characterAndFlags;
};

struct TextSegmentProperties {
    bool isRTL;
    size_t start;
    size_t end;
    TextScript script;

    constexpr TextSegmentProperties(bool isRTL, size_t start, size_t end, TextScript script)
        : isRTL(isRTL), start(start), end(end), script(script) {}
};

struct TextParagraph {
    Valdi::SmallVector<TextSegmentProperties, 2> segments;
    bool baseDirectionIsRightToLeft;
};

using TextParagraphList = Valdi::SmallVector<TextParagraph, 2>;

class TextShaper : public Valdi::SimpleRefCountable {
public:
    virtual void clearCache() {}

    virtual TextParagraphList resolveParagraphs(const Character* unicodeText, size_t length, bool isRightToLeft) = 0;

    virtual size_t shape(const Character* unicodeText,
                         size_t length,
                         Font& font,
                         bool isRightToLeft,
                         Scalar letterSpacing,
                         TextScript script,
                         std::vector<ShapedGlyph>& out) = 0;

    static Ref<TextShaper> make(bool enableCache);
};

} // namespace snap::drawing
