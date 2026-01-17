//
//  FontFamily.hpp
//  snap_drawing-macos
//
//  Created by Simon Corsin on 10/19/22.
//

#pragma once

#include "include/core/SkTypeface.h"
#include "snap_drawing/cpp/Utils/Aliases.hpp"

#include "snap_drawing/cpp/Text/FontStyle.hpp"
#include "valdi_core/cpp/Utils/FlatMap.hpp"

class SkFontMgr;

namespace snap::drawing {

class Typeface;

/**
 A FontFamily holds a set of related typefaces with different styles.
 */
class FontFamily : public Valdi::SimpleRefCountable {
public:
    explicit FontFamily(const String& familyName);
    ~FontFamily() override;

    const String& getFamilyName() const;

    /**
     Returns the closest matching Typeface from the given FontStyle.
     May return null if the FontFamily holds no typefaces.
     */
    virtual Ref<Typeface> matchStyle(SkFontMgr& fontMgr, FontStyle fontStyle) = 0;

private:
    String _familyName;
};

} // namespace snap::drawing
