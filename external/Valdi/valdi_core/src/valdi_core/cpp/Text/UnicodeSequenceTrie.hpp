#pragma once

#include "valdi_core/cpp/Text/CharacterSet.hpp"
#include "valdi_core/cpp/Utils/FlatMap.hpp"
#include "valdi_core/cpp/Utils/SmallVector.hpp"
#include <cstdint>

namespace Valdi {

using UnicodeSequence = SmallVector<uint32_t, 4>;

/**
 * UnicodeSequenceTrie is a class that can build an index of unicode sequence of arbitrarily length.
 * It can be used to efficiently query about whether a unicode point array has unicode sequences
 * that are indexed by this trie. This is used in practice to detect emojis.
 */
class UnicodeSequenceTrie {
public:
    UnicodeSequenceTrie();
    ~UnicodeSequenceTrie();

    void insert(const uint32_t* sequence, size_t length);

    void insert(uint32_t unicodePoint);

    void insert(std::initializer_list<uint32_t> sequence);

    void rebuildIndex();

    /**
     * Find the longest matching sequence for the given unicode array.
     * Return nullptr if no sequences were found.
     */
    const UnicodeSequence* find(const uint32_t* unicodePoints, size_t length) const;

    /**
     * Find the longest matching sequence for the given unicode array.
     * Return nullptr if no sequences were found.
     */
    const UnicodeSequence* find(const uint32_t* unicodePointsBegin, const uint32_t* unicodePointsEnd) const;

    const UnicodeSequence* find(uint32_t unicodePoint) const;

    const UnicodeSequence* find(std::initializer_list<uint32_t> unicodePoints) const;

private:
    struct Entry {
        bool exists = false;
        UnicodeSequence unicodeSequence;
        FlatMap<uint32_t, Entry> children;
        Ref<CharacterSet> characterSet;
    };

    Entry _rootEntry;

    Entry& emplace(Entry& current, size_t index, const uint32_t* sequence, size_t length);
    void buildIndex(Entry& entry);
};

} // namespace Valdi