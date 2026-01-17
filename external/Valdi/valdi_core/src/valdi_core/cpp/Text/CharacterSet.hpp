//
//  CharacterSet.hpp
//  snap_drawing-macos
//
//  Created by Simon Corsin on 10/18/22.
//

#pragma once

#include "valdi_core/cpp/Utils/Shared.hpp"
#include <memory>
#include <vector>

namespace Valdi {

using Character = uint32_t;

struct CharacterRange {
    Character from;
    Character to;

    constexpr CharacterRange(Character from, Character to) : from(from), to(to) {}
};

/**
 A CharacterSet is built by set list of sorted CharacterRange, and can quickly
 tell whether a character is within the set. It does a single heap allocation
 at build time to contain the whole CharacterSet, and can retrieve  whether
 a character is supported in O(1).
 */
class CharacterSet : public Valdi::SimpleRefCountable {
public:
    CharacterSet();
    ~CharacterSet() override;

    bool contains(Character character) const;

    static Ref<CharacterSet> make(const CharacterRange* ranges, size_t length);

    static Ref<CharacterSet> make(std::initializer_list<CharacterRange> ranges) {
        return make(ranges.begin(), ranges.size());
    }

    bool operator==(const CharacterSet& other) const;
    bool operator!=(const CharacterSet& other) const;

private:
    // Could have also used an std::bitset, although it makes the contains()
    // implementation of the CharacterSet about 20% slower compared to just
    // using a raw bitset.
    using Bitset = uint32_t[8];

    // The minimum offset value that this set supports.
    // Resolved offsets are substracted from this value
    uint32_t _minOffset = 0;
    // The maximum offset value that this set supports.
    uint32_t _maxOffset = 0;
    // For each offset, this will contain the index where
    // to find the bitset. Ranges that are empty will point
    // to the first bitset which is always zero.
    // Only non-empty ranges have an actual bitset allocated.
    const uint16_t* _offsets = nullptr;
    const Bitset* _bitsets = nullptr;

    CharacterSet(uint32_t minOffset, uint32_t maxOffset, const uint16_t* offsets, const Bitset* bitsets);

    static void initializeBitsets(const CharacterRange* ranges,
                                  size_t length,
                                  uint32_t minOffset,
                                  uint16_t* offsets,
                                  CharacterSet::Bitset* bitsets);
};

} // namespace Valdi
