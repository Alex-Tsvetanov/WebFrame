#include "valdi/runtime/Text/Emoji.hpp"
#include "valdi_core/cpp/Text/UTF16Utils.hpp"
#include <gtest/gtest.h>

using namespace Valdi;

namespace ValdiTest {

SmallVector<uint32_t, 32> makeUTF32String(const char* str) {
    auto result = utf8ToUtf32(str, strlen(str));
    return SmallVector<uint32_t, 32>(result.first, result.first + result.second);
}

TEST(Emoji, canDetectSingleEmoji) {
    const auto* emojiTrie = getEmojiUnicodeTrie();

    const auto* result = emojiTrie->find({0x23F0});
    ASSERT_TRUE(result != nullptr);
    ASSERT_EQ(UnicodeSequence({0x23F0}), *result);

    result = emojiTrie->find({0x1F190});
    ASSERT_TRUE(result == nullptr);

    result = emojiTrie->find({0x1F191});
    ASSERT_TRUE(result != nullptr);
    ASSERT_EQ(UnicodeSequence({0x1F191}), *result);

    result = emojiTrie->find({0x1F192});
    ASSERT_TRUE(result != nullptr);
    ASSERT_EQ(UnicodeSequence({0x1F192}), *result);

    result = emojiTrie->find({0x1F19A});
    ASSERT_TRUE(result != nullptr);
    ASSERT_EQ(UnicodeSequence({0x1F19A}), *result);

    result = emojiTrie->find({0x1F19B});

    ASSERT_TRUE(result == nullptr);
}

TEST(Emoji, canDetectSingleEmojiInString) {
    const auto* emojiTrie = getEmojiUnicodeTrie();
    auto str = makeUTF32String("?!✅");

    const auto* data = str.data();
    const auto* dataEnd = data + str.size();

    const auto* result = emojiTrie->find(data, dataEnd);
    ASSERT_TRUE(result == nullptr);

    data++;

    result = emojiTrie->find(data, dataEnd);
    ASSERT_TRUE(result == nullptr);
    data++;

    result = emojiTrie->find(data, dataEnd);
    ASSERT_TRUE(result != nullptr);
    data++;

    ASSERT_EQ(UnicodeSequence({0x2705}), *result);

    ASSERT_EQ(data, dataEnd);
}

TEST(Emoji, canDetectEmojiModifier) {
    const auto* emojiTrie = getEmojiUnicodeTrie();

    const auto* result = emojiTrie->find({0x23CF, 0xFE0F});
    ASSERT_TRUE(result != nullptr);

    ASSERT_EQ(UnicodeSequence({0x23CF, 0xFE0F}), *result);

    result = emojiTrie->find({0x2199, 0xFE0F});
    ASSERT_TRUE(result != nullptr);

    ASSERT_EQ(UnicodeSequence({0x2199, 0xFE0F}), *result);

    result = emojiTrie->find({0x21A0, 0xFE0F});
    ASSERT_TRUE(result == nullptr);
}

TEST(Emoji, canDetectComplexEmojis) {
    const auto* emojiTrie = getEmojiUnicodeTrie();

    const auto* result = emojiTrie->find({0x1F3F4, 0xE0067, 0xE0062, 0xE0065, 0xE006E, 0xE0067});
    ASSERT_TRUE(result != nullptr);
    // Should find the black flag emoji
    ASSERT_EQ(UnicodeSequence({0x1F3F4}), *result);

    result = emojiTrie->find({0x1F3F4, 0xE0067, 0xE0062, 0xE0065, 0xE006E, 0xE0067, 0xE007F});
    ASSERT_TRUE(result != nullptr);

    ASSERT_EQ(UnicodeSequence({0x1F3F4, 0xE0067, 0xE0062, 0xE0065, 0xE006E, 0xE0067, 0xE007F}), *result);
}

} // namespace ValdiTest