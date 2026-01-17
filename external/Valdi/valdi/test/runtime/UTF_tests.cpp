#include "valdi_core/cpp/Text/UTF16Utils.hpp"
#include <gtest/gtest.h>
#include <vector>

using namespace Valdi;

namespace ValdiTest {

TEST(UTF16ToUTF32Index, canResolveIndexesOnEqualLength) {
    std::string_view str = "Hello World\n";

    std::vector<uint32_t> utf32;
    std::vector<char16_t> utf16;

    for (auto c : str) {
        utf32.emplace_back(static_cast<uint32_t>(c));
        utf16.emplace_back(static_cast<char16_t>(c));
    }

    UTF16ToUTF32Index index(utf16.data(), utf16.size(), utf32.data(), utf32.size());

    ASSERT_EQ(static_cast<size_t>(0), index.getUTF32Index(0));
    ASSERT_EQ(static_cast<size_t>(5), index.getUTF32Index(5));
    ASSERT_EQ(static_cast<size_t>(11), index.getUTF32Index(11));
    // Out of bounds should still work
    ASSERT_EQ(static_cast<size_t>(42), index.getUTF32Index(42));
}

TEST(UTF16ToUTF32Index, canResolveIndexesOnDifferentLength) {
    std::string_view str = "مقدمة إلى \U0001F385\U0001F385\u2066C++ \U0001F385 ASCII \U0001F385\U0001F385 MORE ASCII "
                           "\U0001F385 \U0001F385 HERE";

    auto utf16 = utf8ToUtf16(str.data(), str.size());
    auto utf32 = utf8ToUtf32(str.data(), str.size());

    ASSERT_EQ(static_cast<size_t>(54), utf16.second);
    ASSERT_EQ(static_cast<size_t>(47), utf32.second);

    UTF16ToUTF32Index index(utf16.first, utf16.second, utf32.first, utf32.second);

    ASSERT_EQ(static_cast<size_t>(0), index.getUTF32Index(0));
    ASSERT_EQ(static_cast<size_t>(4), index.getUTF32Index(4));
    ASSERT_EQ(static_cast<size_t>(4), index.getUTF32Index(4));
    ASSERT_EQ(static_cast<size_t>(9), index.getUTF32Index(9));
    ASSERT_EQ(static_cast<size_t>(10), index.getUTF32Index(10));
    ASSERT_EQ(static_cast<size_t>(10), index.getUTF32Index(11));
    ASSERT_EQ(static_cast<size_t>(11), index.getUTF32Index(12));
    ASSERT_EQ(static_cast<size_t>(11), index.getUTF32Index(13));
    ASSERT_EQ(static_cast<size_t>(12), index.getUTF32Index(14));
    ASSERT_EQ(static_cast<size_t>(13), index.getUTF32Index(15));
    ASSERT_EQ(static_cast<size_t>(17), index.getUTF32Index(19));
    ASSERT_EQ(static_cast<size_t>(17), index.getUTF32Index(20));
    ASSERT_EQ(static_cast<size_t>(25), index.getUTF32Index(28));
    ASSERT_EQ(static_cast<size_t>(25), index.getUTF32Index(29));
}

} // namespace ValdiTest
