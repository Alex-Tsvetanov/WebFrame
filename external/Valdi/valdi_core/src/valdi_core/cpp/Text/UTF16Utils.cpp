//
//  UTF16Utils.cpp
//  valdi-ios
//
//  Created by Simon Corsin on 11/8/18.
//

#include "valdi_core/cpp/Text/UTF16Utils.hpp"
#include "UTF16Utils.hpp"
#include <algorithm>
#include <vector>

namespace Valdi {

/*
 * UTF-8 and UTF-16 conversion functions from miniutf: https://github.com/dropbox/miniutf
 */
struct OffsetPt {
    int offset;
    char32_t pt;
};

static constexpr const OffsetPt kInvalidPt = {-1, 0};

// Code below copied and modifed from djinni, so that it exports to a temp buffer
// instead of creating a new string everytime.

static inline bool isHighSurrogate(char16_t c) {
    return (c >= 0xD800) && (c < 0xDC00);
}
static inline bool isLowSurrogate(char16_t c) {
    return (c >= 0xDC00) && (c < 0xE000);
}

/*
 * Like utf8DecodeCheck, but for UTF-16.
 */
static inline OffsetPt utf16DecodeCheck(const char16_t* str, std::u16string::size_type i) {
    if (isHighSurrogate(str[i]) && isLowSurrogate(str[i + 1])) {
        // High surrogate followed by low surrogate
        char32_t pt = (((str[i] - 0xD800) << 10) | (str[i + 1] - 0xDC00)) + 0x10000;
        return {2, pt};
    } else if (isHighSurrogate(str[i]) || isLowSurrogate(str[i])) {
        // High surrogate *not* followed by low surrogate, or unpaired low surrogate
        return kInvalidPt;
    } else {
        return {1, str[i]};
    }
}

static inline char32_t utf16Decode(const char16_t* str, std::u16string::size_type& i) {
    OffsetPt res = utf16DecodeCheck(str, i);
    if (res.offset < 0) {
        i += 1;
        return 0xFFFD;
    } else {
        i += res.offset;
        return res.pt;
    }
}

static inline void utf8Encode(char32_t pt, std::vector<char>& out) {
    if (pt < 0x80) {
        out.push_back(static_cast<char>(pt));
    } else if (pt < 0x800) {
        out.push_back(static_cast<char>((pt >> 6) | 0xC0));
        out.push_back(static_cast<char>((pt & 0x3F) | 0x80));
    } else if (pt < 0x10000) {
        out.push_back(static_cast<char>((pt >> 12) | 0xE0));
        out.push_back(static_cast<char>(((pt >> 6) & 0x3F) | 0x80));
        out.push_back(static_cast<char>((pt & 0x3F) | 0x80));
    } else if (pt < 0x110000) {
        out.push_back(static_cast<char>((pt >> 18) | 0xF0));
        out.push_back(static_cast<char>(((pt >> 12) & 0x3F) | 0x80));
        out.push_back(static_cast<char>(((pt >> 6) & 0x3F) | 0x80));
        out.push_back(static_cast<char>((pt & 0x3F) | 0x80));
    } else {
        out.push_back(static_cast<char>(0xEF));
        out.push_back(static_cast<char>(0xBF));
        out.push_back(static_cast<char>(0xBD));
    }
}

static inline OffsetPt utf8DecodeCheck(const char* str, std::string::size_type i) {
    uint32_t b0;
    uint32_t b1;
    uint32_t b2;
    uint32_t b3;

    b0 = static_cast<unsigned char>(str[i]);

    if (b0 < 0x80) {
        // 1-byte character
        return {1, b0};
    } else if (b0 < 0xC0) {
        // Unexpected continuation byte
        return kInvalidPt;
    } else if (b0 < 0xE0) {
        // 2-byte character
        if (((b1 = str[i + 1]) & 0xC0) != 0x80)
            return kInvalidPt;

        char32_t pt = (b0 & 0x1F) << 6 | (b1 & 0x3F);
        if (pt < 0x80)
            return kInvalidPt;

        return {2, pt};
    } else if (b0 < 0xF0) {
        // 3-byte character
        if (((b1 = str[i + 1]) & 0xC0) != 0x80)
            return kInvalidPt;
        if (((b2 = str[i + 2]) & 0xC0) != 0x80)
            return kInvalidPt;

        char32_t pt = (b0 & 0x0F) << 12 | (b1 & 0x3F) << 6 | (b2 & 0x3F);
        if (pt < 0x800)
            return kInvalidPt;

        return {3, pt};
    } else if (b0 < 0xF8) {
        // 4-byte character
        if (((b1 = str[i + 1]) & 0xC0) != 0x80)
            return kInvalidPt;
        if (((b2 = str[i + 2]) & 0xC0) != 0x80)
            return kInvalidPt;
        if (((b3 = str[i + 3]) & 0xC0) != 0x80)
            return kInvalidPt;

        char32_t pt = (b0 & 0x0F) << 18 | (b1 & 0x3F) << 12 | (b2 & 0x3F) << 6 | (b3 & 0x3F);
        if (pt < 0x10000 || pt >= 0x110000)
            return kInvalidPt;

        return {4, pt};
    } else {
        // Codepoint out of range
        return kInvalidPt;
    }
}

static inline char32_t utf8Decode(const char* str, std::string::size_type& i) {
    OffsetPt res = utf8DecodeCheck(str, i);
    if (res.offset < 0) {
        i += 1;
        return 0xFFFD;
    } else {
        i += res.offset;
        return res.pt;
    }
}

static inline void utf16Encode(char32_t pt, std::vector<char16_t>& out) {
    if (pt < 0x10000) {
        out.push_back(static_cast<char16_t>(pt));
    } else if (pt < 0x110000) {
        out.push_back(static_cast<char16_t>(((pt - 0x10000) >> 10) + 0xD800));
        out.push_back(static_cast<char16_t>((pt & 0x3FF) + 0xDC00));
    } else {
        out.push_back(0xFFFD);
    }
}

std::pair<const char*, size_t> utf16ToUtf8(const char16_t* utf16String, size_t len) {
    thread_local static std::vector<char> tBuffer;
    auto& buffer = tBuffer;
    buffer.clear();

    for (std::u16string::size_type i = 0; i < len;) {
        utf8Encode(utf16Decode(utf16String, i), buffer);
    }

    return std::make_pair(buffer.data(), buffer.size());
}

std::pair<const char16_t*, size_t> utf8ToUtf16(const char* utf8String, size_t len) {
    thread_local static std::vector<char16_t> tBuffer;
    auto& buffer = tBuffer;
    buffer.clear();

    for (std::string::size_type i = 0; i < len;) {
        utf16Encode(utf8Decode(utf8String, i), buffer);
    }

    return std::make_pair(buffer.data(), buffer.size());
}

std::pair<const uint32_t*, size_t> utf8ToUtf32(const char* utf8String, size_t len) {
    thread_local static std::vector<uint32_t> tBuffer;
    auto& buffer = tBuffer;
    buffer.clear();

    for (std::string::size_type i = 0; i < len;) {
        buffer.emplace_back(utf8Decode(utf8String, i));
    }

    return std::make_pair(buffer.data(), buffer.size());
}

std::pair<const uint32_t*, size_t> utf16ToUtf32(const char16_t* utf16String, size_t len) {
    thread_local static std::vector<uint32_t> tBuffer;
    auto& buffer = tBuffer;
    buffer.clear();

    for (std::u16string::size_type i = 0; i < len;) {
        buffer.emplace_back(utf16Decode(utf16String, i));
    }

    return std::make_pair(buffer.data(), buffer.size());
}

std::pair<const char*, size_t> utf32ToUtf8(const uint32_t* utf32String, size_t length) {
    thread_local static std::vector<char> tBuffer;
    auto& buffer = tBuffer;
    buffer.clear();

    for (std::u16string::size_type i = 0; i < length; i++) {
        utf8Encode(utf32String[i], buffer);
    }

    return std::make_pair(buffer.data(), buffer.size());
}

std::pair<const char16_t*, size_t> utf32ToUtf16(const uint32_t* utf32String, size_t length) {
    thread_local static std::vector<char16_t> tBuffer;
    auto& buffer = tBuffer;

    auto utf16Size = countUtf32ToUtf16(utf32String, length);
    buffer.resize(utf16Size);

    utf32ToUtf16(utf32String, length, buffer.data(), buffer.size());

    return std::make_pair(buffer.data(), buffer.size());
}

size_t countUtf32ToUtf16(uint32_t utf32Character) {
    return utf32Character > 0xFFFF ? 2 : 1;
}

size_t countUtf32ToUtf16(const uint32_t* utf32String, size_t length) {
    size_t utf16Count = 0;

    for (size_t i = 0; i < length; i++) {
        auto c = utf32String[i];
        utf16Count += countUtf32ToUtf16(c);
    }

    return utf16Count;
}

size_t utf32ToUtf16(const uint32_t* utf32String, size_t length, char16_t* output, size_t capacity) {
    size_t utf16Count = 0;

    for (size_t i = 0; i < length; i++) {
        auto c = utf32String[i];
        if (c > 0xFFFF) {
            if (utf16Count + 2 > capacity) {
                return utf16Count;
            }
            output[utf16Count++] = static_cast<char16_t>((c >> 10) + 0xD79C);
            output[utf16Count++] = static_cast<char16_t>((c & 0x3FF) | 0xDC00);
        } else {
            if (utf16Count + 1 > capacity) {
                return utf16Count;
            }
            output[utf16Count++] = static_cast<char16_t>(c);
        }
    }

    return utf16Count;
}

UTF16ToUTF32Index::UTF16ToUTF32Index(const char16_t* /*utf16String*/,
                                     size_t utf16Length,
                                     const uint32_t* utf32String,
                                     size_t utf32Length)
    : _offsets(utf16Length - utf32Length) {
    size_t offsetPosition = 0;
    // We store positions within the UTF32 string where the characters are represented as 2 UTF16 characters
    for (size_t i = 0; i < utf32Length; i++) {
        if (countUtf32ToUtf16(utf32String[i]) == 2) {
            _offsets.data()[offsetPosition++] = i;
        }
    }
}

size_t UTF16ToUTF32Index::getUTF32Index(size_t utf16Index) const {
    size_t utf32Index = utf16Index;

    for (size_t offsetIndex = 0; offsetIndex < _offsets.size(); offsetIndex++) {
        auto utf32Position = _offsets.data()[offsetIndex];

        if (utf32Position >= utf32Index) {
            break;
        }

        utf32Index--;
    }

    return utf32Index;
}

} // namespace Valdi
