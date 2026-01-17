#include "valdi_core/cpp/Text/UnicodeSequenceTrie.hpp"
#include <gtest/gtest.h>

using namespace Valdi;

namespace ValdiTest {

TEST(UniqueSequenceTrie, canInsertAndQuerySingleUnicode) {
    UnicodeSequenceTrie trie;

    trie.insert(0x1F18E);
    trie.insert(0x22);
    trie.insert(0x2611);

    trie.rebuildIndex();

    const auto* result = trie.find(0);
    ASSERT_TRUE(result == nullptr);

    result = trie.find(1);
    ASSERT_TRUE(result == nullptr);

    result = trie.find(0x1F18F);
    ASSERT_TRUE(result == nullptr);

    result = trie.find(0x1F18E);
    ASSERT_TRUE(result != nullptr);

    ASSERT_EQ(UnicodeSequence({0x1F18E}), *result);

    result = trie.find(0x22);
    ASSERT_TRUE(result != nullptr);

    ASSERT_EQ(UnicodeSequence({0x22}), *result);

    result = trie.find(0x2610);
    ASSERT_TRUE(result == nullptr);

    result = trie.find(0x2611);
    ASSERT_TRUE(result != nullptr);

    ASSERT_EQ(UnicodeSequence({0x2611}), *result);

    result = trie.find(0x2612);
    ASSERT_TRUE(result == nullptr);
}

TEST(UniqueSequenceTrie, canInsertAndQueryLongerUnicodeSequence) {
    UnicodeSequenceTrie trie;

    trie.insert({0x0023, 0xFE0F, 0x20E3});
    trie.insert({0x1F6CF, 0xFE0F});

    trie.rebuildIndex();

    const auto* result = trie.find({0x0023});

    ASSERT_TRUE(result == nullptr);

    result = trie.find({0x0023, 0xFE0F});
    ASSERT_TRUE(result == nullptr);

    result = trie.find({0x0023, 0xFE0F, 0x20E3});
    ASSERT_TRUE(result != nullptr);
    ASSERT_EQ(UnicodeSequence({0x0023, 0xFE0F, 0x20E3}), *result);

    result = trie.find({0x1F6CF});
    ASSERT_TRUE(result == nullptr);

    result = trie.find({0x1F6CF, 0xFE0F});
    ASSERT_TRUE(result != nullptr);
    ASSERT_EQ(UnicodeSequence({0x1F6CF, 0xFE0F}), *result);
}

TEST(UnicodeSequenceTrie, canFindSmallerSequences) {
    UnicodeSequenceTrie trie;

    trie.insert({0x0023, 0xFE0F});
    trie.insert({0x0023, 0xFE0F, 0x20E3});

    trie.rebuildIndex();

    const auto* result = trie.find({0x0023});

    ASSERT_TRUE(result == nullptr);

    result = trie.find({0x0023, 0xFE0F});
    ASSERT_TRUE(result != nullptr);
    ASSERT_EQ(UnicodeSequence({0x0023, 0xFE0F}), *result);

    result = trie.find({0x0023, 0xFE0F, 0x20E2});
    ASSERT_TRUE(result != nullptr);
    ASSERT_EQ(UnicodeSequence({0x0023, 0xFE0F}), *result);

    result = trie.find({0x0023, 0xFE0F, 0x20E3});
    ASSERT_TRUE(result != nullptr);
    ASSERT_EQ(UnicodeSequence({0x0023, 0xFE0F, 0x20E3}), *result);
}

} // namespace ValdiTest