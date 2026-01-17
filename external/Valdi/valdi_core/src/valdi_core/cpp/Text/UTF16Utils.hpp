//
//  UTF16Utils.hpp
//  valdi-ios
//
//  Created by Simon Corsin on 11/8/18.
//

#pragma once

#include "valdi_core/cpp/Utils/AutoMalloc.hpp"
#include <string>
#include <utility>

namespace Valdi {

/**
 * Takes a UTF16 string and return a pair on a temporary buffer containing
 * the converted string and its UTF8 length. Buffer lifecycle is tied to the thread.
 * Subsequent calls to this utf16ToUtf8 will override the buffer.
 */
std::pair<const char*, size_t> utf16ToUtf8(const char16_t* utf16String, size_t len);

/**
 * Takes a UTF8 string and return a pair on a temporary buffer containing
 * the converted string and its UTF16 length. Buffer lifecycle is tied to the thread.
 * Subsequent calls to this utf8ToUtf16 will override the buffer.
 */
std::pair<const char16_t*, size_t> utf8ToUtf16(const char* utf8String, size_t len);

/**
 * Takes a UTF8 string and return a pair on a temporary buffer containing
 * the converted string and its UTF32 length. Buffer lifecycle is tied to the thread.
 * Subsequent calls to this utf8ToUtf32 will override the buffer.
 */
std::pair<const uint32_t*, size_t> utf8ToUtf32(const char* utf8String, size_t len);

/**
 * Takes a UTF16 string and return a pair on a temporary buffer containing
 * the converted string and its UTF32 length. Buffer lifecycle is tied to the thread.
 * Subsequent calls to this utf8ToUtf32 will override the buffer.
 */
std::pair<const uint32_t*, size_t> utf16ToUtf32(const char16_t* utf16String, size_t len);

/**
 * Takes a UTF32 string and return a pair on a temporary buffer containing
 * the converted string and its UTF8 length. Buffer lifecycle is tied to the thread.
 * Subsequent calls to this utf32ToUtf8 will override the buffer.
 */
std::pair<const char*, size_t> utf32ToUtf8(const uint32_t* utf32String, size_t length);

/**
 * Takes a UTF32 string and return a pair on a temporary buffer containing
 * the converted string and its UTF16 length. Buffer lifecycle is tied to the thread.
 * Subsequent calls to this utf32ToUtf16 will override the buffer.
 */
std::pair<const char16_t*, size_t> utf32ToUtf16(const uint32_t* utf32String, size_t length);

/**
 Returns the number of UTF16 characters needed to convert the given UTF32 string
 */
size_t countUtf32ToUtf16(const uint32_t* utf32String, size_t length);

/**
 Returns the number of UTF16 characters needed to convert the given UTF32 character
 */
size_t countUtf32ToUtf16(uint32_t utf32Character);

/**
 Converts a UTF32 string in UTF16 into the given buffer.
 Returns how many characters we written into the output.
 */
size_t utf32ToUtf16(const uint32_t* utf32String, size_t length, char16_t* output, size_t capacity);

/**
 An Index that can efficiently resolve a UTF32 index from a UTF16 index.
 This can be helpful when integrating between libraries that deal with different
 string encoding.
 The algorithm complexity is O(k), where K is the number of characters within the
 UTF32 strings that need to be encoded as 2 characters in UTF16.
 */
class UTF16ToUTF32Index {
public:
    UTF16ToUTF32Index(const char16_t* utf16String, size_t utf16Length, const uint32_t* utf32String, size_t utf32Length);

    size_t getUTF32Index(size_t utf16Index) const;

private:
    AutoMalloc<size_t, 16> _offsets;
};

} // namespace Valdi
