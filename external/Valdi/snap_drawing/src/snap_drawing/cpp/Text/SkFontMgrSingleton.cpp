#include "snap_drawing/cpp/Text/SkFontMgrSingleton.hpp"

#if defined(SK_FONTMGR_CORETEXT_AVAILABLE)

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wimport-preprocessor-directive-pedantic"
#include "include/ports/SkFontMgr_mac_ct.h"
#pragma clang diagnostic pop

#elif defined(SK_FONTMGR_ANDROID_AVAILABLE)
#include "include/ports/SkFontMgr_android.h"
#include "include/ports/SkFontScanner_FreeType.h"
#elif defined(SK_FONTMGR_FONTCONFIG_AVAILABLE)
#include "include/ports/SkFontMgr_fontconfig.h"
#include "include/ports/SkFontScanner_FreeType.h"
#else
#include "include/ports/SkFontMgr_empty.h"
#endif

namespace snap::snap_drawing {

static sk_sp<SkFontMgr> makeSkFontMgr() {
    // If necessary, clients should use a font manager that would load fonts from the system.
#if defined(SK_FONTMGR_CORETEXT_AVAILABLE)
    return SkFontMgr_New_CoreText(nullptr);
#elif defined(SK_FONTMGR_ANDROID_AVAILABLE)
    return SkFontMgr_New_Android(nullptr, SkFontScanner_Make_FreeType());
#elif defined(SK_FONTMGR_FONTCONFIG_AVAILABLE)
    return SkFontMgr_New_FontConfig(nullptr, SkFontScanner_Make_FreeType());
#else
    return SkFontMgr_New_Custom_Empty();
#endif
}

sk_sp<SkFontMgr> getSkFontMgrSingleton() {
    static auto kFontMgr = makeSkFontMgr();
    return kFontMgr;
}

} // namespace snap::snap_drawing