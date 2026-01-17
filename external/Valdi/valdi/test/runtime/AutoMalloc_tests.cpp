#include "valdi_core/cpp/Utils/AutoMalloc.hpp"
#include <gtest/gtest.h>

using namespace Valdi;

namespace ValdiTest {

TEST(AutoMalloc, doesntAllocateWhenCapacityIsLargeEnough) {
    AutoMalloc<uint32_t, 16> storage(8);

    ASSERT_FALSE(storage.allocated());

    ASSERT_TRUE(storage.data() != nullptr);

    for (size_t i = 0; i < 8; i++) {
        storage.data()[i] = 42;
    }

    for (size_t i = 0; i < 8; i++) {
        ASSERT_EQ(static_cast<uint32_t>(42), storage.data()[i]);
    }
}

TEST(AutoMalloc, allocatesWhenCapacityIsNotLargeEnough) {
    AutoMalloc<uint32_t, 16> storage(32);

    ASSERT_TRUE(storage.allocated());

    ASSERT_TRUE(storage.data() != nullptr);

    for (size_t i = 0; i < 32; i++) {
        storage.data()[i] = 42;
    }

    for (size_t i = 0; i < 32; i++) {
        ASSERT_EQ(static_cast<uint32_t>(42), storage.data()[i]);
    }
}

TEST(AutoMalloc, canBeResized) {
    AutoMalloc<uint32_t, 16> storage;

    ASSERT_FALSE(storage.allocated());
    ASSERT_TRUE(storage.data() == nullptr);

    storage.resize(8);

    ASSERT_FALSE(storage.allocated());
    ASSERT_TRUE(storage.data() != nullptr);

    auto* currentPtr = storage.data();

    storage.resize(32);

    ASSERT_TRUE(storage.allocated());
    ASSERT_TRUE(storage.data() != nullptr);

    ASSERT_NE(currentPtr, storage.data());

    currentPtr = storage.data();

    storage.resize(30);

    ASSERT_TRUE(storage.allocated());
    ASSERT_EQ(currentPtr, storage.data());

    storage.resize(31);

    ASSERT_TRUE(storage.allocated());
    ASSERT_EQ(currentPtr, storage.data());
}

} // namespace ValdiTest
