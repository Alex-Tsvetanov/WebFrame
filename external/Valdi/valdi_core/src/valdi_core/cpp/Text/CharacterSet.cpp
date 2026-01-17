//
//  CharacterSet.cpp
//  snap_drawing-macos
//
//  Created by Simon Corsin on 10/18/22.
//

#include "valdi_core/cpp/Text/CharacterSet.hpp"
#include "valdi_core/cpp/Utils/InlineContainerAllocator.hpp"

#include <iostream>
#include <optional>
#include <utility>

namespace Valdi {

static constexpr uint32_t kAllOnes = ~static_cast<uint32_t>(0);

// Unicode is a 21 bits encoding. This will index by pages that are 13 bits,
// which we can store in a 16 bits integer.
static constexpr uint32_t kOffsetShift = 8;
static constexpr uint32_t kOffsetMask = ~((kAllOnes >> kOffsetShift) << kOffsetShift);

// Once we shift to resolve the page, there is 8 bits of data remaining, the maximum
// number in 8 bits being 255, we need 256 bits per bitsets to store that information.
// We use 8 32bits integer as an array to store the bitset.
// We shift the remaining 8 bits of the page by 5 to get the index from within that
// 32bits integer array.
static constexpr uint32_t kBitsetShift = 5;
static constexpr uint32_t kBitsetMask = ~((kAllOnes >> kBitsetShift) << kBitsetShift);

CharacterSet::CharacterSet() = default;

CharacterSet::CharacterSet(uint32_t minOffset, uint32_t maxOffset, const uint16_t* offsets, const Bitset* bitsets)
    : _minOffset(minOffset), _maxOffset(maxOffset), _offsets(offsets), _bitsets(bitsets) {}

CharacterSet::~CharacterSet() = default;

bool CharacterSet::contains(Character character) const {
    auto c = static_cast<uint32_t>(character);
    auto offsetIndex = c >> kOffsetShift;
    auto offsetValue = c & kOffsetMask;
    auto indexInBitset = offsetValue >> kBitsetShift;

    if (offsetIndex < _minOffset || offsetIndex > _maxOffset) {
        return false;
    }

    auto offset = _offsets[offsetIndex - _minOffset];
    const auto& bitset = _bitsets[offset];
    uint32_t expectedBit = 1 << (offsetValue & kBitsetMask);

    return (bitset[indexInBitset] & expectedBit) != 0;
}

template<typename F>
static void processCharacters(const CharacterRange* ranges, size_t length, F&& fn) {
    std::optional<uint32_t> lastOffsetIndex;
    uint32_t bitsetsCount = 0;

    for (size_t i = 0; i < length; i++) {
        const auto& range = ranges[i];

        auto from = range.from;
        auto to = range.to;
        for (uint32_t j = from; j <= to; j++) {
            auto offsetIndex = j >> kOffsetShift;
            auto offsetValue = j & kOffsetMask;
            if (!lastOffsetIndex || lastOffsetIndex.value() != offsetIndex) {
                lastOffsetIndex = {offsetIndex};
                bitsetsCount++;
            }

            fn(offsetIndex, bitsetsCount, offsetValue);
        }
    }
}

void CharacterSet::initializeBitsets(
    const CharacterRange* ranges, size_t length, uint32_t minOffset, uint16_t* offsets, CharacterSet::Bitset* bitsets) {
    processCharacters(ranges, length, [&](uint32_t offsetIndex, uint32_t bitsetsCount, uint32_t offsetValue) {
        offsets[offsetIndex - minOffset] = bitsetsCount;

        auto& bitset = bitsets[bitsetsCount];

        auto indexInBitset = offsetValue >> kBitsetShift;
        auto valueInBitset = 1 << (offsetValue & kBitsetMask);

        bitset[indexInBitset] |= valueInBitset;
    });
}

static bool needsSort(const CharacterRange* ranges, size_t length) {
    for (size_t i = 1; i < length; i++) {
        if (ranges[i - 1].from > ranges[i].from) {
            return true;
        }
    }

    return false;
}

Ref<CharacterSet> CharacterSet::make(const CharacterRange* ranges, size_t length) {
    std::vector<CharacterRange> sortedRanges;
    if (needsSort(ranges, length)) {
        sortedRanges = std::vector<CharacterRange>(ranges, ranges + length);
        std::sort(sortedRanges.begin(), sortedRanges.end(), [](const auto& left, const auto& right) {
            return left.from < right.from;
        });

        ranges = sortedRanges.data();
    }

    std::optional<uint32_t> minOffsetIndex;
    uint32_t lastOffsetIndex = 0;
    uint32_t resolvedBitsetsCount = 0;
    processCharacters(ranges, length, [&](uint32_t offsetIndex, uint32_t bitsetsCount, uint32_t /*offsetValue*/) {
        if (!minOffsetIndex || minOffsetIndex.value() > offsetIndex) {
            minOffsetIndex = {offsetIndex};
        }
        resolvedBitsetsCount = bitsetsCount;
        lastOffsetIndex = std::max(lastOffsetIndex, offsetIndex);
    });

    auto resolvedMinOffsetIndex = minOffsetIndex ? minOffsetIndex.value() : 0;
    auto totalOffsetsCount = (lastOffsetIndex - resolvedMinOffsetIndex) + 1;
    auto totalBitsetsCount = resolvedBitsetsCount + 1;

    auto offsetsAllocSize = totalOffsetsCount * sizeof(uint16_t);
    auto bitsetsAllocSize = totalBitsetsCount * sizeof(Bitset);

    auto baseAllocSize = sizeof(CharacterSet);
    auto withOffsetsAllocSize = Valdi::alignUp(baseAllocSize + offsetsAllocSize, alignof(uint16_t));
    auto withBitsetsAllocSize = Valdi::alignUp(withOffsetsAllocSize + bitsetsAllocSize, alignof(Bitset));

    std::allocator<uint8_t> allocator;
    auto* region = allocator.allocate(withBitsetsAllocSize);
    std::memset(region, 0, withBitsetsAllocSize);

    auto* offsets = reinterpret_cast<uint16_t*>(&region[withOffsetsAllocSize - offsetsAllocSize]);
    auto* bitsets = reinterpret_cast<Bitset*>(&region[withBitsetsAllocSize - bitsetsAllocSize]);

    initializeBitsets(ranges, length, resolvedMinOffsetIndex, offsets, bitsets);

    new (region)(CharacterSet)(resolvedMinOffsetIndex, lastOffsetIndex, offsets, bitsets);

    return Ref<CharacterSet>(reinterpret_cast<CharacterSet*>(region));
}

bool CharacterSet::operator==(const CharacterSet& other) const {
    if (_minOffset != other._minOffset || _maxOffset != other._maxOffset) {
        return false;
    }

    for (size_t i = _minOffset; i <= _maxOffset; i++) {
        const auto& bitsetFrom = _bitsets[i - _minOffset];
        const auto& bitsetTo = other._bitsets[i - _minOffset];

        for (size_t j = 0; j < sizeof(Bitset) / sizeof(uint32_t); j++) {
            if (bitsetFrom[j] != bitsetTo[j]) {
                return false;
            }
        }
    }

    return true;
}

bool CharacterSet::operator!=(const CharacterSet& other) const {
    return !(*this == other);
}

} // namespace Valdi
