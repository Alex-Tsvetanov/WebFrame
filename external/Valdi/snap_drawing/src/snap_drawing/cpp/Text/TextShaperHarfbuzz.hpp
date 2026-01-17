//
//  TextShaperHarfbuzz.hpp
//  snap_drawing
//
//  Created by Simon Corsin on 3/9/22.
//

#pragma once

#include "snap_drawing/cpp/Text/Harfbuzz.hpp"
#include "snap_drawing/cpp/Text/TextShaper.hpp"
#include "valdi_core/cpp/Utils/Mutex.hpp"

namespace snap::drawing {

struct CachedHBFace;

class TextShaperHarfbuzz : public TextShaper {
public:
    TextShaperHarfbuzz();
    ~TextShaperHarfbuzz() override;

    TextParagraphList resolveParagraphs(const Character* unicodeText, size_t length, bool isRightToLeft) override;

    size_t shape(const Character* unicodeText,
                 size_t length,
                 Font& font,
                 bool isRightToLeft,
                 Scalar letterSpacing,
                 TextScript script,
                 std::vector<ShapedGlyph>& out) override;

private:
    Valdi::Mutex _mutex;
    HBBuffer _buffer;

    static TextParagraphList resolveParagraphsPrimitive(const Character* unicodeText,
                                                        size_t length,
                                                        bool isRightToLeft);
    static TextParagraphList resolveParagraphsICU(const Character* unicodeText, size_t length, bool isRightToLeft);
};

} // namespace snap::drawing
