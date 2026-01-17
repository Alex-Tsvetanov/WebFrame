//
//  FontFamilyWithStyleSet.cpp
//  snap_drawing
//
//  Created by Simon Corsin on 12/12/22.
//

#include "snap_drawing/cpp/Text/FontFamilyWithStyleSet.hpp"
#include "snap_drawing/cpp/Text/Typeface.hpp"

#include "include/core/SkFontMgr.h"

namespace snap::drawing {

FontFamilyWithStyleSet::FontFamilyWithStyleSet(const sk_sp<SkFontStyleSet>& fontStyleSet, const String& familyName)
    : FontFamily(familyName), _fontStyleSet(fontStyleSet) {}
FontFamilyWithStyleSet::~FontFamilyWithStyleSet() = default;

Ref<Typeface> FontFamilyWithStyleSet::matchStyle(SkFontMgr& fontMgr, FontStyle fontStyle) {
    const auto& it = _fontIdByStyle.find(fontStyle);
    if (it != _fontIdByStyle.end()) {
        if (it->second == 0) {
            return nullptr;
        }

        return _typefaces[it->second];
    }

    sk_sp<SkTypeface> matchedTypeface(_fontStyleSet->matchStyle(fontStyle.getSkValue()));
    if (matchedTypeface == nullptr) {
        _fontIdByStyle[fontStyle] = 0;
        return nullptr;
    }

    auto id = matchedTypeface->uniqueID();
    _fontIdByStyle[fontStyle] = id;

    const auto& typefaceId = _typefaces.find(id);
    if (typefaceId != _typefaces.end()) {
        return typefaceId->second;
    }

    auto typeface = Valdi::makeShared<Typeface>(std::move(matchedTypeface), getFamilyName(), false);
    _typefaces[id] = typeface;

    return typeface;
}

} // namespace snap::drawing
