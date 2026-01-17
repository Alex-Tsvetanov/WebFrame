//
//  StaticValueArray_tests.cpp
//  valdi-pc
//
//  Created by Simon Corsin on 5/13/20.
//

#include "valdi_core/cpp/Utils/StringCache.hpp"
#include "valdi_core/cpp/Utils/ValueArray.hpp"
#include "valdi_core/cpp/Utils/ValueArrayBuilder.hpp"
#include "valdi_core/cpp/Utils/ValueMap.hpp"
#include <gtest/gtest.h>

using namespace Valdi;

namespace ValdiTest {

TEST(StaticValueArray, respondsToSize) {
    auto array = ValueArray::make(4);

    ASSERT_EQ(static_cast<size_t>(4), array->size());
}

TEST(StaticValueArray, canInsertAndGetItem) {
    auto array = ValueArray::make(3);

    ASSERT_EQ(ValueType::Null, (*array)[0].getType());
    ASSERT_EQ(ValueType::Null, (*array)[1].getType());
    ASSERT_EQ(ValueType::Null, (*array)[2].getType());

    (*array)[0] = Value(static_cast<int32_t>(42));
    (*array)[1] = Value(static_cast<double>(42.5));
    (*array)[2] = Value(STRING_LITERAL("hello"));

    ASSERT_TRUE((*array)[0].isInt());
    ASSERT_TRUE((*array)[1].isDouble());
    ASSERT_TRUE((*array)[2].isString());

    ASSERT_EQ(static_cast<int32_t>(42), (*array)[0].toInt());
    ASSERT_EQ(static_cast<double>(42.5), (*array)[1].toDouble());
    ASSERT_EQ(STRING_LITERAL("hello"), (*array)[2].toStringBox());
}

TEST(ValueArray, retainAndReleaseItems) {
    auto map = makeShared<ValueMap>();

    ASSERT_EQ(1, map.use_count());

    auto array = ValueArray::make(1);
    (*array)[0] = Value(map);

    ASSERT_EQ(2, map.use_count());

    (*array)[0] = Value();

    ASSERT_EQ(1, map.use_count());
}

TEST(ValueArray, releaseItemsOnDestruction) {
    auto map = makeShared<ValueMap>();

    auto array = ValueArray::make(1);
    (*array)[0] = Value(map);

    ASSERT_EQ(2, map.use_count());

    array = nullptr;

    ASSERT_EQ(1, map.use_count());
}

TEST(ValueArray, canIterateThrough) {
    auto array = ValueArray::make(3);

    (*array)[0] = Value(static_cast<int32_t>(42));
    (*array)[1] = Value(static_cast<double>(42.5));
    (*array)[2] = Value(STRING_LITERAL("hello"));

    std::vector<Value> iteratedItems;

    for (const auto& it : *array) {
        iteratedItems.emplace_back(it);
    }

    ASSERT_EQ(static_cast<size_t>(3), iteratedItems.size());
    ASSERT_EQ(static_cast<int32_t>(42), iteratedItems[0].toInt());
    ASSERT_EQ(static_cast<double>(42.5), iteratedItems[1].toDouble());
    ASSERT_EQ(STRING_LITERAL("hello"), iteratedItems[2].toStringBox());
}

TEST(ValueArray, canCreateFromInitializer) {
    auto array = ValueArray::make({Value(static_cast<int32_t>(42)), Value(42.5), Value(STRING_LITERAL("hello"))});

    ASSERT_EQ(static_cast<size_t>(3), array->size());
    ASSERT_EQ(static_cast<int32_t>(42), (*array)[0].toInt());
    ASSERT_EQ(static_cast<double>(42.5), (*array)[1].toDouble());
    ASSERT_EQ(STRING_LITERAL("hello"), (*array)[2].toStringBox());
}

TEST(ValueArray, canCompare) {
    auto left = ValueArray::make(1);
    left->emplace(0, Value(42));

    auto right = ValueArray::make(1);
    right->emplace(0, Value(42));

    ASSERT_EQ(*left, *right);

    right = ValueArray::make(2);
    right->emplace(0, Value(42));

    ASSERT_NE(*left, *right);

    left = ValueArray::make(2);
    left->emplace(0, Value(42));

    ASSERT_EQ(*left, *right);

    right->emplace(1, Value(STRING_LITERAL("hello")));

    ASSERT_NE(*left, *right);

    left->emplace(1, Value(STRING_LITERAL("hello")));

    ASSERT_EQ(*left, *right);
}

TEST(ValueArray, canClone) {
    auto left = ValueArray::make(2);
    left->emplace(0, Value(42));
    left->emplace(1, Value(true));

    auto right = left->clone();

    ASSERT_EQ(left->size(), right->size());
    ASSERT_EQ(static_cast<size_t>(2), left->size());

    ASSERT_EQ((*left)[0], (*right)[0]);
    ASSERT_EQ((*left)[1], (*right)[1]);
    ASSERT_EQ(*left, *right);
}

TEST(ValueArray, canCopy) {
    auto left = ValueArray::make(4);

    std::array<Value, 2> array;
    array[0] = Value(42);
    array[1] = Value(true);

    ASSERT_TRUE((*left)[0].isNull());
    ASSERT_TRUE((*left)[1].isNull());
    ASSERT_TRUE((*left)[2].isNull());
    ASSERT_TRUE((*left)[3].isNull());

    left->copy(left->begin() + 1, array.data(), array.data() + array.size());

    ASSERT_TRUE((*left)[0].isNull());
    ASSERT_TRUE((*left)[1].isInt());
    ASSERT_TRUE((*left)[2].isBool());
    ASSERT_TRUE((*left)[3].isNull());

    ASSERT_EQ(42, (*left)[1].toInt());
    ASSERT_EQ(true, (*left)[2].toBool());
}

TEST(ValueArrayBuilder, canCreateArray) {
    ValueArrayBuilder builder;
    builder.emplace(true);
    builder.emplace(static_cast<int32_t>(42));

    auto array = builder.build();

    ASSERT_EQ(static_cast<size_t>(2), builder.size());

    ASSERT_EQ(*ValueArray::make({Value(true), Value(static_cast<int32_t>(42))}), *array);
}

TEST(ValueArrayBuilder, respondsToSize) {
    ValueArrayBuilder builder;
    builder.emplace(true);
    builder.emplace(static_cast<int32_t>(42));

    ASSERT_EQ(static_cast<size_t>(2), builder.size());
}

TEST(ValueArrayBuilder, canExtendCapacity) {
    ValueArrayBuilder builder;
    builder.emplace(true);
    builder.emplace(static_cast<int32_t>(42));

    ASSERT_EQ(static_cast<size_t>(2), builder.size());
    ASSERT_EQ(static_cast<size_t>(4), builder.capacity());

    builder.emplace(true);
    builder.emplace(true);
    builder.emplace(true);

    ASSERT_EQ(static_cast<size_t>(5), builder.size());
    ASSERT_EQ(static_cast<size_t>(8), builder.capacity());
}

TEST(ValueArrayBuilder, canCreateArrayWithBigSize) {
    ValueArrayBuilder builder;

    for (int32_t i = 0; i < 50; i++) {
        builder.emplace(Value(i));
    }

    ASSERT_EQ(static_cast<size_t>(50), builder.size());
    ASSERT_EQ(static_cast<size_t>(64), builder.capacity());

    auto array = builder.build();

    ASSERT_EQ(static_cast<size_t>(50), array->size());

    int32_t i = 0;
    for (const auto& value : *array) {
        ASSERT_TRUE(value.isInt());
        ASSERT_EQ(i, value.toInt());
        i++;
    }
}

TEST(ValueArrayBuilder, canRemove) {
    ValueArrayBuilder builder;
    builder.emplace(true);
    builder.emplace(static_cast<int32_t>(42));
    builder.emplace(static_cast<int32_t>(84));
    builder.emplace(static_cast<int32_t>(150));

    ASSERT_EQ(static_cast<size_t>(4), builder.size());

    // Remove from end
    builder.remove(3);
    ASSERT_EQ(static_cast<size_t>(3), builder.size());

    auto array = builder.build();

    ASSERT_EQ(*ValueArray::make({Value(true), Value(42), Value(84)}), *array);

    // Remove from middle

    builder.remove(1);

    ASSERT_EQ(static_cast<size_t>(2), builder.size());

    array = builder.build();

    ASSERT_EQ(*ValueArray::make({Value(true), Value(84)}), *array);

    // Remove from start

    builder.remove(0);

    ASSERT_EQ(static_cast<size_t>(1), builder.size());

    array = builder.build();

    ASSERT_EQ(*ValueArray::make({Value(84)}), *array);
}

} // namespace ValdiTest
