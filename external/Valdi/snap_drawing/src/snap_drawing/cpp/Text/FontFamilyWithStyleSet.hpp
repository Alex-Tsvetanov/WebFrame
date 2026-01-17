//
//  FontFamilyWithStyleSet.hpp
//  snap_drawing
//
//  Created by Simon Corsin on 12/12/22.
//

#pragma once

#include "snap_drawing/cpp/Text/FontFamily.hpp"
#include "valdi_core/cpp/Utils/FlatMap.hpp"

class SkFontStyleSet;

namespace snap::drawing {

class Typeface;

/**
 A FontFamily implementation that is backed by a Skia SkFontStyleSet instance.
 */
class FontFamilyWithStyleSet : public FontFamily {
public:
    FontFamilyWithStyleSet(const sk_sp<SkFontStyleSet>& fontStyleSet, const String& familyName);
    ~FontFamilyWithStyleSet() override;

    Ref<Typeface> matchStyle(SkFontMgr& fontMgr, FontStyle fontStyle) override;

private:
    sk_sp<SkFontStyleSet> _fontStyleSet;
    Valdi::FlatMap<SkTypefaceID, Ref<Typeface>> _typefaces;
    Valdi::FlatMap<FontStyle, SkTypefaceID> _fontIdByStyle;
};

} // namespace snap::drawing
