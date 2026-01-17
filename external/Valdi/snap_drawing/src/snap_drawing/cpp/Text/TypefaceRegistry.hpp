//
//  TypefaceRegistry.hpp
//  valdi-skia
//
//  Created by Simon Corsin on 6/13/24.
//

#pragma once

#include "snap_drawing/cpp/Text/Character.hpp"
#include "snap_drawing/cpp/Text/FontStyle.hpp"
#include "snap_drawing/cpp/Utils/Aliases.hpp"

#include "valdi_core/cpp/Utils/FlatMap.hpp"
#include "valdi_core/cpp/Utils/Result.hpp"

#include <vector>

class SkFontMgr;

namespace Valdi {
class ILogger;
}

namespace snap::drawing {

class TextShaper;
class FontFamily;
class FontFamilyWithLoadableTypefaces;
class LoadableTypeface;
class Typeface;

class ITypefaceRegistryListener {
public:
    virtual ~ITypefaceRegistryListener() = default;

    /**
     Called whenever a FontFamily was not found within the TypefaceRegistry. It gives an opportunity
     to the listener to resolve and return the missing FontFamily.
     */
    virtual Ref<FontFamily> onFontFamilyMissing(const String& familyName) = 0;
};

class TypefaceRegistry {
public:
    TypefaceRegistry(Valdi::ILogger& logger, ITypefaceRegistryListener* listener);
    ~TypefaceRegistry();

    std::vector<String> getAllAvailableTypefaceNames() const;

    Valdi::Result<Valdi::Ref<Typeface>> getTypefaceWithName(SkFontMgr& fontMgr,
                                                            const String& fontName,
                                                            FontStyle* outResolvedFontStyle);
    Valdi::Result<Valdi::Ref<Typeface>> getTypefaceWithFamilyNameAndStyle(SkFontMgr& fontMgr,
                                                                          const String& familyName,
                                                                          FontStyle fontStyle);

    Valdi::Ref<FontFamily> getFontFamilyByFamilyName(const String& familyName);

    Valdi::Ref<FontFamily> getFontFamilyForCharacterAndStyle(SkFontMgr& fontMgr,
                                                             Character character,
                                                             FontStyle fontStyle);

    Valdi::Result<Valdi::Ref<Typeface>> getTypefaceFromFamily(SkFontMgr& fontMgr,
                                                              const Ref<FontFamily>& fontFamily,
                                                              FontStyle fontStyle);

    void registerFamilyName(const String& familyName);

    /**
     Registers a Typeface from a loadable typeface. This will make the typeface available through the various
     getTypeface methods.
     */
    void registerTypeface(const String& fontFamilyName,
                          FontStyle fontStyle,
                          bool canUseAsFallback,
                          const Ref<LoadableTypeface>& loadableTypeface);

    Valdi::ILogger& getLogger() const;

private:
    Valdi::ILogger& _logger;
    ITypefaceRegistryListener* _listener;
    Valdi::FlatMap<String, String> _familyNameByFontKey;
    Valdi::FlatMap<String, Ref<FontFamily>> _fontFamilyByFontKey;
    Valdi::FlatMap<String, Ref<FontFamily>> _fontFamilyByFamilyName;
    Valdi::FlatMap<String, Ref<FontFamily>> _fontFamilyByFallbackKey;
    std::vector<Ref<FontFamily>> _registeredFallbackFontFamilies;

    Ref<FontFamilyWithLoadableTypefaces> getOrCreateFontFamilyWithLoadableTypefaces(const String& fontFamilyName);

    Valdi::Ref<FontFamily> getFontFamilyByFamilyName(Valdi::FlatMap<String, Ref<FontFamily>>& fontFamilyMap,
                                                     const String& familyName);
};

} // namespace snap::drawing
