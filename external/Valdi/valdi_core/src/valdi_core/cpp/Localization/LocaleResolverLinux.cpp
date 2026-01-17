#include "valdi_core/cpp/Localization/LocaleResolver.hpp"

#if !defined(__APPLE__)

#include <cstdlib>

namespace Valdi {

static StringBox getEnv(const char* name) {
    const char* v = std::getenv(name);
    if (v == nullptr) {
        return StringBox();
    }
    return StringBox::fromCString(v);
}

std::vector<StringBox> LocaleResolver::getPreferredLocales() {
    // 1. LC_ALL has absolute priority and is a single locale
    auto lcAll = getEnv("LC_ALL");
    if (!lcAll.isEmpty()) {
        return {lcAll};
    }

    // 2. LANGUAGE may contain a colon‑separated *list*
    auto language = getEnv("LANGUAGE");
    if (!language.isEmpty()) {
        return language.split(':');
    }

    // 3. Category‑specific override
    auto lcMessages = getEnv("LC_MESSAGES");
    if (!lcMessages.isEmpty()) {
        return {lcMessages};
    }

    // 4. Global default
    auto lang = getEnv("LANG");
    if (!lang.isEmpty()) {
        return {lang};
    }

    return {};
}

} // namespace Valdi

#endif