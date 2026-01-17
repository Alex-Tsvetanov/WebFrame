//
//  HexUtils_tests.cpp
//  valdi-pc
//
//  Created by Simon Corsin on 4/9/20.
//

#include <gtest/gtest.h>

#include "valdi/runtime/Utils/HexUtils.hpp"
#include <array>

using namespace Valdi;

// NOTE: Most of the tests are already done in PersistentStoreTest.ts
// Those tests are testing more specific logic that are invisible to the
// consumers of the store.

namespace ValdiTest {

TEST(HexUtils, canConvertHexToString) {
    std::array<Byte, 2> bytes = {244, 45};

    auto result = bytesToHexString(bytes.data(), bytes.size());

    ASSERT_EQ("f42d", result);
}

TEST(HexUtils, insertPaddingWhenConvertingToString) {
    std::array<Byte, 4> bytes = {244, 0, 45, 5};
    std::array<Byte, 4> emptyBytes = {0, 0, 0, 0};

    auto result = bytesToHexString(bytes.data(), bytes.size());
    auto result2 = bytesToHexString(emptyBytes.data(), emptyBytes.size());

    ASSERT_EQ("f4002d05", result);
    ASSERT_EQ("00000000", result2);
}

TEST(HexUtils, canConvertStringToArray) {
    std::array<Byte, 2> bytes;
    auto read = hexStringToBytes("f42d", bytes.data(), bytes.size());

    ASSERT_EQ(static_cast<size_t>(4), read);
    ASSERT_EQ(244, bytes[0]);
    ASSERT_EQ(45, bytes[1]);
}

TEST(HexUtils, canConvertStringToArrayWithPadding) {
    std::array<Byte, 4> bytes;
    auto read = hexStringToBytes("0a0010f0", bytes.data(), bytes.size());

    ASSERT_EQ(static_cast<size_t>(8), read);
    ASSERT_EQ(10, bytes[0]);
    ASSERT_EQ(0, bytes[1]);
    ASSERT_EQ(16, bytes[2]);
    ASSERT_EQ(240, bytes[3]);
}

TEST(HexUtils, stopsReadingWhenReachingTheArrayBounds) {
    std::array<Byte, 3> bytes;
    auto read = hexStringToBytes("f42df42df42df42df42df42df42df42d", bytes.data(), bytes.size());

    ASSERT_EQ(static_cast<size_t>(6), read);
}

TEST(HexUtils, returnsZeroWhenArrayIsBiggerThanString) {
    std::array<Byte, 16> bytes;
    auto read = hexStringToBytes("f42d", bytes.data(), bytes.size());

    ASSERT_EQ(static_cast<size_t>(0), read);
}

} // namespace ValdiTest
