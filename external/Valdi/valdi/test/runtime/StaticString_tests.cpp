//
//  StaticString_tests.cpp
//  valdi-pc
//
//  Created by Simon Corsin on 2/22/23.
//

#include <gtest/gtest.h>
#include <string_view>

#include "valdi_core/cpp/Utils/StaticString.hpp"

using namespace Valdi;

// NOTE: Most of the tests are already done in PersistentStoreTest.ts
// Those tests are testing more specific logic that are invisible to the
// consumers of the store.

namespace ValdiTest {

TEST(StaticString, canCreateUTF8String) {
    std::string myString = "Hello World";

    auto staticString = StaticString::makeUTF8(myString.data(), myString.size());

    ASSERT_EQ(std::string_view(myString), std::string_view(staticString->utf8Data(), staticString->size()));
}

TEST(StaticString, nullTerminatesUTF8String) {
    std::string myString = "Hello World";

    auto staticString = StaticString::makeUTF8(myString.data(), myString.size());

    ASSERT_EQ(0, staticString->utf8Data()[myString.size()]);
}

TEST(StaticString, canCreateUTF16String) {
    std::u16string myString = u"Hello World";

    auto staticString = StaticString::makeUTF16(myString.data(), myString.size());

    ASSERT_EQ(std::u16string_view(myString), std::u16string_view(staticString->utf16Data(), staticString->size()));
}

static std::vector<char32_t> makeUtf32String(std::string_view asciiString) {
    std::vector<char32_t> utf32String;
    for (auto c : asciiString) {
        utf32String.emplace_back(static_cast<char32_t>(c));
    }
    return utf32String;
}

TEST(StaticString, canCreateUTF32String) {
    auto string = makeUtf32String("Hello World");

    auto staticString = StaticString::makeUTF32(string.data(), string.size());

    ASSERT_EQ(std::u32string_view(string.data(), string.size()),
              std::u32string_view(staticString->utf32Data(), staticString->size()));
}

TEST(StaticString, canGetUTF8Storage) {
    std::string myString = "Hello World";

    auto staticString = StaticString::makeUTF8(myString.data(), myString.size());

    auto utf8Storage = staticString->utf8Storage();

    // When going from UTF8 to UTF8, it should return the underlying storage
    ASSERT_EQ(utf8Storage.data, staticString->utf8Data());

    std::u16string myUTF16String = u"Hello World";

    staticString = StaticString::makeUTF16(myUTF16String.data(), myUTF16String.size());

    utf8Storage = staticString->utf8Storage();

    ASSERT_NE(utf8Storage.data, staticString->utf8Data());

    ASSERT_EQ(std::string_view("Hello World"), std::string_view(utf8Storage.data, utf8Storage.length));

    auto utf32String = makeUtf32String("Hello World");

    staticString = StaticString::makeUTF32(utf32String.data(), utf32String.size());

    utf8Storage = staticString->utf8Storage();

    ASSERT_NE(utf8Storage.data, staticString->utf8Data());

    ASSERT_EQ(std::string_view("Hello World"), std::string_view(utf8Storage.data, utf8Storage.length));
}

TEST(StaticString, canGetUTF16Storage) {
    std::u16string myUTF16String = u"Hello World";

    auto staticString = StaticString::makeUTF16(myUTF16String.data(), myUTF16String.size());

    auto utf16Storage = staticString->utf16Storage();

    // When going from UTF16 to UTF16, it should return the underlying storage
    ASSERT_EQ(utf16Storage.data, staticString->utf16Data());

    std::string myString = "Hello World";

    staticString = StaticString::makeUTF8(myString.data(), myString.size());

    utf16Storage = staticString->utf16Storage();

    ASSERT_NE(utf16Storage.data, staticString->utf16Data());

    ASSERT_EQ(std::u16string_view(u"Hello World"), std::u16string_view(utf16Storage.data, utf16Storage.length));

    auto utf32String = makeUtf32String("Hello World");

    staticString = StaticString::makeUTF32(utf32String.data(), utf32String.size());

    utf16Storage = staticString->utf16Storage();

    ASSERT_NE(utf16Storage.data, staticString->utf16Data());

    ASSERT_EQ(std::u16string_view(u"Hello World"), std::u16string_view(utf16Storage.data, utf16Storage.length));
}

TEST(StaticString, canGetUTF32Storage) {
    auto utf32String = makeUtf32String("Hello World");

    auto staticString = StaticString::makeUTF32(utf32String.data(), utf32String.size());

    auto utf32Storage = staticString->utf32Storage();

    // When going from UTF32 to UTF32, it should return the underlying storage
    ASSERT_EQ(utf32Storage.data, staticString->utf32Data());

    std::string myString = "Hello World";

    staticString = StaticString::makeUTF8(myString.data(), myString.size());

    utf32Storage = staticString->utf32Storage();

    ASSERT_NE(utf32Storage.data, staticString->utf32Data());

    ASSERT_EQ(std::u32string_view(utf32String.data(), utf32String.size()),
              std::u32string_view(utf32Storage.data, utf32Storage.length));

    std::u16string myUTF16String = u"Hello World";

    staticString = StaticString::makeUTF16(myUTF16String.data(), myUTF16String.size());

    utf32Storage = staticString->utf32Storage();

    ASSERT_NE(utf32Storage.data, staticString->utf32Data());

    ASSERT_EQ(std::u32string_view(utf32String.data(), utf32String.size()),
              std::u32string_view(utf32Storage.data, utf32Storage.length));
}

TEST(StaticString, canCompareString) {
    std::string myString = "Hello World";
    std::string myString2 = "Hell0 W0rld";

    auto utf8 = StaticString::makeUTF8(myString.data(), myString.size());
    auto utf8Second = StaticString::makeUTF8(myString.data(), myString.size());
    auto utf8Third = StaticString::makeUTF8(myString2.data(), myString2.size());

    std::u16string myUTF16String = u"Hello World";
    std::u16string myUTF16String2 = u"Hell0 W0rld";

    auto utf16 = StaticString::makeUTF16(myUTF16String.data(), myUTF16String.size());
    auto utf16Second = StaticString::makeUTF16(myUTF16String.data(), myUTF16String.size());
    auto utf16Third = StaticString::makeUTF16(myUTF16String2.data(), myUTF16String2.size());

    ASSERT_EQ(*utf8, *utf8);
    ASSERT_EQ(*utf8, *utf8Second);
    ASSERT_NE(*utf8, *utf8Third);

    ASSERT_NE(*utf8, *utf16);
    ASSERT_NE(*utf8, *utf16Second);
    ASSERT_NE(*utf8, *utf16Third);

    ASSERT_EQ(*utf16, *utf16);
    ASSERT_EQ(*utf16, *utf16Second);
    ASSERT_NE(*utf16, *utf16Third);

    ASSERT_NE(*utf16, *utf8);
    ASSERT_NE(*utf16, *utf8Second);
    ASSERT_NE(*utf16, *utf8Third);
}

} // namespace ValdiTest
