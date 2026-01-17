#include "valdi/runtime/Text/Emoji.hpp"
#include "valdi/runtime/Text/Emoji_Gen.hpp"
#include "valdi_core/cpp/Text/UnicodeSequenceTrie.hpp"

namespace Valdi {

constexpr uint32_t kSINGLE_CODEPOINT_OPCODE = 0x1;
constexpr uint32_t kEMOJI_SEQUENCE_OPCODE = 0x2;
constexpr uint32_t kCOMPLEX_EMOJI_SEQUENCE_OPCODE = 0x3;
constexpr uint32_t kUnicodePointMask = ~static_cast<uint32_t>(0) >> 11;

static UnicodeSequenceTrie* makeEmojiUnicodeTrie() {
    auto* trie = new UnicodeSequenceTrie();
    SmallVector<uint32_t, 8> tmp;

    const uint32_t* begin = kVALDI_EMOJI_COMPRESSED_TABLE.data();
    const uint32_t* end = begin + kVALDI_EMOJI_COMPRESSED_TABLE.size();

    const auto* it = begin;
    while (it != end) {
        auto current = *it;
        it++;

        auto opcode = (current >> 29);
        auto sequenceLength = (current >> 21) & 0xFF;
        auto unicodePoint = current & kUnicodePointMask;

        if (opcode == kSINGLE_CODEPOINT_OPCODE) {
            for (uint32_t i = unicodePoint; i < unicodePoint + sequenceLength; i++) {
                trie->insert(i);
            }
        } else if (opcode == kEMOJI_SEQUENCE_OPCODE) {
            for (size_t i = 0; i < sequenceLength; i++) {
                auto entry = *it;
                auto entriesNum = (entry >> 21) & 0xFF;
                auto entryUnicodePoint = entry & kUnicodePointMask;
                it++;

                for (uint32_t i = entryUnicodePoint; i < entryUnicodePoint + entriesNum; i++) {
                    uint32_t sequence[2] = {i, unicodePoint};
                    trie->insert(sequence, 2);
                }
            }
        } else if (opcode == kCOMPLEX_EMOJI_SEQUENCE_OPCODE) {
            tmp.clear();
            tmp.emplace_back(unicodePoint);
            for (size_t i = 1; i < sequenceLength; i++) {
                auto nextUnicodePoint = *it;
                it++;
                tmp.emplace_back(nextUnicodePoint);
            }

            trie->insert(tmp.data(), tmp.size());
        } else {
            std::abort();
        }
    }

    trie->rebuildIndex();

    return trie;
}

const UnicodeSequenceTrie* getEmojiUnicodeTrie() {
    static auto* kTrie = makeEmojiUnicodeTrie();
    return kTrie;
}

} // namespace Valdi