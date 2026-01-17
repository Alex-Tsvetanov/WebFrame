//
//  IFontManager.hpp
//  snap_drawing
//
//  Created by Simon Corsin on 5/16/24.
//

#pragma once

#include "snap_drawing/cpp/Text/Font.hpp"

namespace snap::drawing {

class LoadableTypeface;

class IFontManager : public FontResolver {
public:
    virtual void registerTypeface(const String& fontFamilyName,
                                  FontStyle fontStyle,
                                  bool canUseAsFallback,
                                  const Ref<LoadableTypeface>& loadableTypeface) = 0;

    virtual Valdi::Ref<IFontManager> makeScoped() = 0;

    virtual Valdi::Result<Valdi::Ref<Typeface>> getTypefaceWithName(const String& name) = 0;

    virtual Valdi::Result<Valdi::Ref<Typeface>> getTypefaceWithFamilyNameAndStyle(const String& familyName,
                                                                                  FontStyle fontStyle) = 0;

    VALDI_CLASS_HEADER(IFontManager)
};

} // namespace snap::drawing
