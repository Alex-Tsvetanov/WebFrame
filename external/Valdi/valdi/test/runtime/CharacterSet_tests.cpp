#include <gtest/gtest.h>

#include "utils/time/StopWatch.hpp"
#include "valdi_core/cpp/Text/CharacterSet.hpp"

namespace Valdi {

TEST(CharacterSet, supportsEmpty) {
    auto characterSet = CharacterSet::make({});

    ASSERT_FALSE(characterSet->contains(0));
    ASSERT_FALSE(characterSet->contains(1));
    ASSERT_FALSE(characterSet->contains(0xFFFFFF));
}

TEST(CharacterSet, supportsSingleRange) {
    auto characterSet = CharacterSet::make({CharacterRange(40, 60)});

    Character i = 0;
    while (i < 40) {
        ASSERT_FALSE(characterSet->contains(i++));
    }

    while (i <= 60) {
        ASSERT_TRUE(characterSet->contains(i++));
    }

    while (i <= 2000) {
        ASSERT_FALSE(characterSet->contains(i++));
    }
}

TEST(CharacterSet, supportsMultipleRanges) {
    auto characterSet = CharacterSet::make(
        {CharacterRange(0, 1), CharacterRange(10, 20), CharacterRange(40, 60), CharacterRange(4000, 4200)});

    Character i = 0;
    while (i <= 1) {
        ASSERT_TRUE(characterSet->contains(i++));
    }

    while (i < 10) {
        ASSERT_FALSE(characterSet->contains(i++));
    }

    while (i <= 20) {
        ASSERT_TRUE(characterSet->contains(i++));
    }

    while (i < 40) {
        ASSERT_FALSE(characterSet->contains(i++));
    }

    while (i <= 60) {
        ASSERT_TRUE(characterSet->contains(i++));
    }

    while (i < 4000) {
        ASSERT_FALSE(characterSet->contains(i++));
    }

    while (i <= 4200) {
        ASSERT_TRUE(characterSet->contains(i++));
    }

    while (i < 10000) {
        ASSERT_FALSE(characterSet->contains(i++));
    }
}

TEST(CharacterSet, returnsEmptyOnOutOfBounds) {
    auto characterSet = CharacterSet::make(
        {CharacterRange(0, 1), CharacterRange(10, 20), CharacterRange(40, 60), CharacterRange(4000, 0xFFFFFF)});

    ASSERT_TRUE(characterSet->contains(0));
    ASSERT_TRUE(characterSet->contains(15));
    ASSERT_FALSE(characterSet->contains(0xFFFFFF + 1));
}

TEST(CharacterSet, supportsUnorderedInsertions) {
    auto characterSet =
        CharacterSet::make({CharacterRange(1000, 1001), CharacterRange(0, 1), CharacterRange(1010, 1012)});

    ASSERT_TRUE(characterSet->contains(0));
    ASSERT_TRUE(characterSet->contains(1));
    ASSERT_FALSE(characterSet->contains(2));

    ASSERT_TRUE(characterSet->contains(1000));
    ASSERT_TRUE(characterSet->contains(1001));
    ASSERT_FALSE(characterSet->contains(1002));
    ASSERT_TRUE(characterSet->contains(1010));
    ASSERT_TRUE(characterSet->contains(1011));
    ASSERT_TRUE(characterSet->contains(1012));
    ASSERT_FALSE(characterSet->contains(1013));
}

} // namespace Valdi
