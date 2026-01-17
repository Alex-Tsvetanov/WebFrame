#include "valdi_core/cpp/Localization/LocaleResolver.hpp"

#if defined(__APPLE__)

#include "valdi_core/cpp/Apple/CFConversions.hpp"

#include <CoreFoundation/CoreFoundation.h>

namespace Valdi {

std::vector<StringBox> LocaleResolver::getPreferredLocales() {
    auto locales = CFRef<CFArrayRef>(CFLocaleCopyPreferredLanguages());
    auto count = CFArrayGetCount(locales.get());
    std::vector<StringBox> output;
    output.reserve(static_cast<size_t>(count));
    for (CFIndex i = 0; i < count; i++) {
        auto str = reinterpret_cast<CFStringRef>(CFArrayGetValueAtIndex(locales.get(), i));
        output.emplace_back(toStringBox(str));
    }

    return output;
}

} // namespace Valdi

#endif