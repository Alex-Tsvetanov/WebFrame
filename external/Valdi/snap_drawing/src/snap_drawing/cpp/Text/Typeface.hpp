//
//  Typeface.hpp
//  snap_drawing-macos
//
//  Created by Simon Corsin on 1/28/22.
//

#pragma once

#include "snap_drawing/cpp/Text/Character.hpp"
#include "snap_drawing/cpp/Text/FontMetrics.hpp"
#include "snap_drawing/cpp/Text/FontStyle.hpp"
#include "snap_drawing/cpp/Text/Harfbuzz.hpp"
#include "snap_drawing/cpp/Utils/Aliases.hpp"

#include "valdi_core/cpp/Utils/Bytes.hpp"
#include "valdi_core/cpp/Utils/FlatMap.hpp"
#include "valdi_core/cpp/Utils/Mutex.hpp"

#include "include/core/SkTypeface.h"

namespace Valdi {
class CharacterSet;
}

namespace snap::drawing {

class Typeface : public Valdi::SimpleRefCountable {
public:
    Typeface(sk_sp<SkTypeface>&& typeface, const String& familyName, bool isCustom);
    ~Typeface() override;

    uint32_t getId() const;

    const sk_sp<SkTypeface>& getSkValue() const;

    const String& familyName() const;
    const FontStyle& fontStyle() const;

    bool isCustom() const;

    bool hasSpaceInLigaturesOrKerning() const;

    bool isEmoji() const;

    bool supportsCharacter(Character character);

    Valdi::BytesView makeFontData();

    const HBFace& getHBFace() const;
    const HBFont& getHBFont() const;

    FontMetrics getFontMetrics(double fontSize);

private:
    mutable Valdi::Mutex _mutex;
    sk_sp<SkTypeface> _typeface;
    Ref<Valdi::CharacterSet> _characterSet;
    String _familyName;
    FontStyle _fontStyle;
    bool _isCustom;
    bool _isEmoji;
    HBFace _hbFace;
    HBFont _hbFont;
    Valdi::FlatMap<double, FontMetrics> _fontMetricsBySize;
};

} // namespace snap::drawing
