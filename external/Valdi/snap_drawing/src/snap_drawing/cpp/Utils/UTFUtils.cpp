//
//  UTFUtils.cpp
//  snap_drawing
//
//  Created by Simon Corsin on 3/16/22.
//

#include "snap_drawing/cpp/Utils/UTFUtils.hpp"
#include "include/core/SkTypes.h"

namespace SkUTF {

// Those APIs exist in Skia but not in the public headers
SK_SPI int CountUTF8(const char* utf8, size_t byteLength);    // NOLINT
SK_SPI SkUnichar NextUTF8(const char** ptr, const char* end); // NOLINT
SK_SPI size_t ToUTF8(SkUnichar uni, char* utf8);              // NOLINT

} // namespace SkUTF

namespace snap::drawing {

std::vector<Character> utf8ToUnicode(std::string_view utf8) {
    std::vector<Character> out;
    utf8ToUnicode(utf8, out);
    return out;
}

size_t utf8ToUnicode(std::string_view utf8, std::vector<Character>& output) {
    const auto* utf8Start = utf8.data();

    auto unicodeCount = SkUTF::CountUTF8(utf8Start, utf8.size());
    if (unicodeCount <= 0) {
        return 0;
    }

    auto previousSize = output.size();
    output.resize(previousSize + unicodeCount);

    auto* unicodePtr = reinterpret_cast<SkUnichar*>(output.data() + previousSize);

    const auto* utf8End = utf8Start + utf8.size();

    while (utf8Start < utf8End) {
        (*unicodePtr) = SkUTF::NextUTF8(&utf8Start, utf8End);
        unicodePtr++;
    }

    return unicodeCount;
}

std::string unicodeToUtf8(const Character* unicode, size_t length) {
    std::string characters;
    char buffer[4];

    const auto* skCharactersPtr = reinterpret_cast<const SkUnichar*>(unicode);

    for (size_t i = 0; i < length; i++) {
        size_t written = SkUTF::ToUTF8(skCharactersPtr[i], buffer);
        characters.append(buffer, written);
    }

    return characters;
}

} // namespace snap::drawing
