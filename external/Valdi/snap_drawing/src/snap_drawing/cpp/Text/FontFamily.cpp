//
//  FontFamily.cpp
//  snap_drawing-macos
//
//  Created by Simon Corsin on 10/19/22.
//

#include "snap_drawing/cpp/Text/FontFamily.hpp"
#include "snap_drawing/cpp/Text/Typeface.hpp"

#include "include/core/SkFontMgr.h"

namespace snap::drawing {

FontFamily::FontFamily(const String& familyName) : _familyName(familyName) {}
FontFamily::~FontFamily() = default;

const String& FontFamily::getFamilyName() const {
    return _familyName;
}

} // namespace snap::drawing
