//
//  TypefaceRegistry.cpp
//  valdi-skia
//
//  Created by Simon Corsin on 6/13/24.
//

#include "snap_drawing/cpp/Text/TypefaceRegistry.hpp"
#include "snap_drawing/cpp/Text/FontFamily.hpp"
#include "snap_drawing/cpp/Text/FontFamilyWithLoadableTypefaces.hpp"
#include "snap_drawing/cpp/Text/FontFamilyWithStyleSet.hpp"
#include "snap_drawing/cpp/Text/Typeface.hpp"

#include "valdi_core/cpp/Utils/LoggerUtils.hpp"
#include "valdi_core/cpp/Utils/StringCache.hpp"

namespace snap::drawing {

static String toFontKey(const String& fontName) {
    auto fontKey = fontName.lowercased();
    if (fontKey.contains(' ')) {
        auto components = fontKey.split(' ', true);
        fontKey = Valdi::StringBox::join(components, "");
    }

    return fontKey;
}

static String toFamilyName(const String& fontName) {
    auto separatorIndex = fontName.lastIndexOf('-');
    return separatorIndex ? fontName.split(*separatorIndex).first : fontName;
}

static Valdi::Result<std::pair<String, FontStyle>> resolveFontKeyAndStyle(const String& fontName) {
    auto fontKey = toFontKey(fontName);
    auto fontWeight = FontWeightNormal;

    auto familyNameAndWeight = FontStyle::getFamilyNameAndFontWeight(fontKey);
    if (!familyNameAndWeight) {
        return familyNameAndWeight.error().rethrow(
            STRING_FORMAT("Error resolving family name and weight '{}'", fontKey));
    }
    std::tie(fontKey, fontWeight) = familyNameAndWeight.value();

    return std::make_pair(fontKey, FontStyle(FontWidthNormal, fontWeight, FontSlantUpright));
}

TypefaceRegistry::TypefaceRegistry(Valdi::ILogger& logger, ITypefaceRegistryListener* listener)
    : _logger(logger), _listener(listener) {}
TypefaceRegistry::~TypefaceRegistry() = default;

Valdi::Result<Valdi::Ref<Typeface>> TypefaceRegistry::getTypefaceWithName(SkFontMgr& fontMgr,
                                                                          const String& fontName,
                                                                          FontStyle* outResolvedFontStyle) {
    auto fontKeyAndStyleResult = resolveFontKeyAndStyle(fontName);
    if (!fontKeyAndStyleResult) {
        return fontKeyAndStyleResult.moveError();
    }

    auto fontKey = std::move(fontKeyAndStyleResult.value().first);
    auto fontStyle = fontKeyAndStyleResult.value().second;
    if (outResolvedFontStyle != nullptr) {
        *outResolvedFontStyle = fontStyle;
    }

    auto fontFamilyIt = _fontFamilyByFontKey.find(fontKey);

    if (fontFamilyIt == _fontFamilyByFontKey.end()) {
        auto familyNameIt = _familyNameByFontKey.find(fontKey);
        auto familyName = familyNameIt == _familyNameByFontKey.end() ? toFamilyName(fontName) : familyNameIt->second;
        return getTypefaceWithFamilyNameAndStyle(fontMgr, familyName, fontStyle);
    }

    return getTypefaceFromFamily(fontMgr, fontFamilyIt->second, fontStyle);
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
Valdi::Result<Valdi::Ref<Typeface>> TypefaceRegistry::getTypefaceFromFamily(SkFontMgr& fontMgr,
                                                                            const Ref<FontFamily>& fontFamily,
                                                                            FontStyle fontStyle) {
    if (fontFamily == nullptr) {
        return Valdi::Error("Could not resolve Font");
    }

    auto typeface = fontFamily->matchStyle(fontMgr, fontStyle);
    if (typeface == nullptr) {
        return Valdi::Error(
            STRING_FORMAT("Could not resolve Font with given style in family {}", fontFamily->getFamilyName()));
    }
    return typeface;
}

Valdi::Result<Valdi::Ref<Typeface>> TypefaceRegistry::getTypefaceWithFamilyNameAndStyle(SkFontMgr& fontMgr,
                                                                                        const String& familyName,
                                                                                        FontStyle fontStyle) {
    return getTypefaceFromFamily(fontMgr, getFontFamilyByFamilyName(familyName), fontStyle);
}

Valdi::Ref<FontFamily> TypefaceRegistry::getFontFamilyByFamilyName(const String& familyName) {
    if (familyName.hasSuffix("##fallback")) {
        return getFontFamilyByFamilyName(_fontFamilyByFallbackKey, familyName);
    } else {
        return getFontFamilyByFamilyName(_fontFamilyByFamilyName, familyName);
    }
}

void TypefaceRegistry::registerTypeface(const String& fontFamilyName,
                                        FontStyle fontStyle,
                                        bool canUseAsFallback,
                                        const Ref<LoadableTypeface>& loadableTypeface) {
    auto fontFamily = getOrCreateFontFamilyWithLoadableTypefaces(fontFamilyName);
    fontFamily->registerLoadableTypeface(fontStyle, loadableTypeface);

    if (canUseAsFallback &&
        std::find(_registeredFallbackFontFamilies.begin(), _registeredFallbackFontFamilies.end(), fontFamily) ==
            _registeredFallbackFontFamilies.end()) {
        _registeredFallbackFontFamilies.emplace_back(fontFamily);
    }
}

void TypefaceRegistry::registerFamilyName(const String& familyName) {
    auto fontKey = toFontKey(familyName);
    _familyNameByFontKey[fontKey] = familyName;
}

Valdi::Ref<FontFamily> TypefaceRegistry::getFontFamilyForCharacterAndStyle(SkFontMgr& fontMgr,
                                                                           Character character,
                                                                           FontStyle fontStyle) {
    for (const auto& fontFamily : _registeredFallbackFontFamilies) {
        // Attempt to lookup in the registered font families before hitting SkFontMgr::matchFamilyStyleCharacter
        auto typeface = fontFamily->matchStyle(fontMgr, fontStyle);
        if (typeface != nullptr && typeface->supportsCharacter(character)) {
            return fontFamily;
        }
    }

    for (const auto& it : _fontFamilyByFallbackKey) {
        // Attempt to lookup in our cache before hitting SkFontMgr::matchFamilyStyleCharacter if we had
        // some fallback fonts provided. This would happen only on Android.
        auto typeface = it.second->matchStyle(fontMgr, fontStyle);
        if (typeface != nullptr && typeface->supportsCharacter(character)) {
            return it.second;
        }
    }

    return nullptr;
}

Valdi::Ref<FontFamily> TypefaceRegistry::getFontFamilyByFamilyName(
    Valdi::FlatMap<String, Ref<FontFamily>>& fontFamilyMap, const String& familyName) {
    Ref<FontFamily> fontFamily;

    const auto& resultByFamilyName = fontFamilyMap.find(familyName);
    if (resultByFamilyName != fontFamilyMap.end()) {
        fontFamily = resultByFamilyName->second;
    } else {
        auto fontKey = toFontKey(familyName);
        const auto& resultByFontKey = _fontFamilyByFontKey.find(fontKey);
        if (resultByFontKey != _fontFamilyByFontKey.end()) {
            fontFamily = resultByFontKey->second;
        } else {
            if (_listener != nullptr) {
                fontFamily = _listener->onFontFamilyMissing(familyName);
                fontFamilyMap[familyName] = fontFamily;
                _fontFamilyByFontKey[fontKey] = fontFamily;
            }
        }
    }
    return fontFamily;
}

Ref<FontFamilyWithLoadableTypefaces> TypefaceRegistry::getOrCreateFontFamilyWithLoadableTypefaces(
    const String& fontFamilyName) {
    auto fontKey = toFontKey(fontFamilyName);

    Ref<FontFamilyWithLoadableTypefaces> fontFamily;
    const auto& it = _fontFamilyByFontKey.find(fontKey);
    if (it != _fontFamilyByFontKey.end()) {
        fontFamily = Valdi::castOrNull<FontFamilyWithLoadableTypefaces>(it->second);
    }

    if (fontFamily == nullptr) {
        fontFamily = Valdi::makeShared<FontFamilyWithLoadableTypefaces>(_logger, fontFamilyName);
        _fontFamilyByFontKey[fontKey] = fontFamily;
    }

    return fontFamily;
}

std::vector<String> TypefaceRegistry::getAllAvailableTypefaceNames() const {
    std::vector<String> availableFonts;
    availableFonts.reserve(_fontFamilyByFamilyName.size());

    for (const auto& it : _fontFamilyByFamilyName) {
        availableFonts.emplace_back(it.first);
    }

    for (const auto& it : _fontFamilyByFallbackKey) {
        availableFonts.emplace_back(it.first);
    }

    return availableFonts;
}

Valdi::ILogger& TypefaceRegistry::getLogger() const {
    return _logger;
}

} // namespace snap::drawing
