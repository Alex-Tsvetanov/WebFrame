//
//  FontFamilyWithLoadableTypefaces.hpp
//  snap_drawing
//
//  Created by Simon Corsin on 12/12/22.
//

#include "snap_drawing/cpp/Text/FontFamily.hpp"
#include "valdi_core/cpp/Utils/FlatMap.hpp"

class SkFontStyleSet;

namespace Valdi {
class ILogger;
}

namespace snap::drawing {

class LoadableTypeface;

/**
 An implementation of FontFamily which holds a list of LoadableTypeface that
 can be externally registered. The LoadableTypeface are loaded lazily when
 they are selected through matchStyle(). If the LoadableTypeface fails to load,
 it will be unregistered from the FontFamily.
 */
class FontFamilyWithLoadableTypefaces : public FontFamily {
public:
    FontFamilyWithLoadableTypefaces(Valdi::ILogger& logger, const String& familyName);
    ~FontFamilyWithLoadableTypefaces() override;

    /**
     Register a LoadableTypeface associated with the given FontStyle. The LoadableTypeface will
     become a candidate for matchStyle().
     */
    void registerLoadableTypeface(FontStyle fontStyle, const Ref<LoadableTypeface>& loadableTypeface);

    Ref<Typeface> matchStyle(SkFontMgr& fontMgr, FontStyle fontStyle) override;

private:
    [[maybe_unused]] Valdi::ILogger& _logger;
    Valdi::FlatMap<FontStyle, Ref<LoadableTypeface>> _typefaces;

    Ref<LoadableTypeface> resolveLoadableTypeface(FontStyle fontStyle) const;
    void unregisterLoadableTypeface(const Ref<LoadableTypeface>& loadableTypeface);
};

} // namespace snap::drawing
