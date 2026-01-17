#include "valdi_core/cpp/Schema/ValueSchema.hpp"
#include "valdi_core/cpp/Utils/Marshaller.hpp"
#include "valdi_core/cpp/Utils/StringCache.hpp"
#include "valdi_core/cpp/Utils/ValueArrayBuilder.hpp"
#include "valdi_core/cpp/Utils/ValueTypedObject.hpp"
#include "gtest/gtest.h"

using namespace Valdi;

namespace ValdiTest {

TEST(Marshaller, canPushAndPopValues) {
    SimpleExceptionTracker exceptionTracker;
    Marshaller marshaller(exceptionTracker);

    ASSERT_EQ(0, marshaller.size());

    marshaller.push(Value(1));
    marshaller.push(Value(2));
    marshaller.push(Value(3));

    ASSERT_EQ(3, marshaller.size());

    marshaller.pop();

    ASSERT_TRUE(exceptionTracker);

    ASSERT_EQ(2, marshaller.size());

    marshaller.pop();
    ASSERT_TRUE(exceptionTracker);
    marshaller.pop();
    ASSERT_TRUE(exceptionTracker);

    ASSERT_EQ(0, marshaller.size());
}

TEST(Marshaller, canSafelyGetUndefined) {
    SimpleExceptionTracker exceptionTracker;
    Marshaller marshaller(exceptionTracker);

    ASSERT_EQ(Value::undefined(), marshaller.getOrUndefined(0));

    marshaller.push(Value(1));

    ASSERT_NE(Value::undefined(), marshaller.getOrUndefined(0));
    ASSERT_NE(Value::undefined(), marshaller.getOrUndefined(-1));

    ASSERT_EQ(Value::undefined(), marshaller.getOrUndefined(1));
    ASSERT_EQ(Value::undefined(), marshaller.getOrUndefined(-2));
}

TEST(Marshaller, canRetrieveValuesByNegativeIndexes) {
    SimpleExceptionTracker exceptionTracker;
    Marshaller marshaller(exceptionTracker);

    marshaller.push(Value(1));
    marshaller.push(Value(2));
    marshaller.push(Value(3));

    ASSERT_EQ(3, marshaller.getInt(-1));
    ASSERT_EQ(2, marshaller.getInt(-2));
    ASSERT_EQ(1, marshaller.getInt(-3));
}

TEST(Marshaller, canRetrieveValuesByPositiveIndexes) {
    SimpleExceptionTracker exceptionTracker;
    Marshaller marshaller(exceptionTracker);

    marshaller.push(Value(1));
    marshaller.push(Value(2));
    marshaller.push(Value(3));

    ASSERT_EQ(1, marshaller.getInt(0));
    ASSERT_EQ(2, marshaller.getInt(1));
    ASSERT_EQ(3, marshaller.getInt(2));
}

TEST(Marshaller, failsOnOutOfBounds) {
    SimpleExceptionTracker exceptionTracker;
    Marshaller marshaller(exceptionTracker);

    ASSERT_TRUE(exceptionTracker);
    marshaller.get(0);
    ASSERT_FALSE(exceptionTracker);
    exceptionTracker.clearError();

    ASSERT_TRUE(exceptionTracker);
    marshaller.get(-1);
    ASSERT_FALSE(exceptionTracker);
    exceptionTracker.clearError();

    ASSERT_TRUE(exceptionTracker);
    marshaller.get(1);
    ASSERT_FALSE(exceptionTracker);
    exceptionTracker.clearError();

    marshaller.push(Value(1));
    marshaller.push(Value(2));
    marshaller.push(Value(3));

    ASSERT_TRUE(exceptionTracker);
    marshaller.get(-4);
    ASSERT_FALSE(exceptionTracker);
    exceptionTracker.clearError();

    ASSERT_TRUE(exceptionTracker);
    marshaller.get(4);
    ASSERT_FALSE(exceptionTracker);
    exceptionTracker.clearError();
}

TEST(Marshaller, canBuildMap) {
    SimpleExceptionTracker exceptionTracker;
    Marshaller marshaller(exceptionTracker);

    auto objectIndex = marshaller.pushMap(0);

    marshaller.push(Value(42));
    marshaller.putMapProperty(STRING_LITERAL("magicNumber"), objectIndex);
    ASSERT_TRUE(exceptionTracker);
    marshaller.push(Value(STRING_LITERAL("Simon")));
    marshaller.putMapProperty(STRING_LITERAL("name"), objectIndex);
    ASSERT_TRUE(exceptionTracker);

    ASSERT_EQ(1, marshaller.size());

    auto object = marshaller.getMap(0);
    ASSERT_NE(nullptr, object);

    auto expectedObject = makeShared<ValueMap>();
    (*expectedObject)[STRING_LITERAL("magicNumber")] = Value(42);
    (*expectedObject)[STRING_LITERAL("name")] = Value(STRING_LITERAL("Simon"));

    ASSERT_EQ(*expectedObject, *object);
}

TEST(Marshaller, canGetMapProperty) {
    SimpleExceptionTracker exceptionTracker;
    Marshaller marshaller(exceptionTracker);

    auto objectIndex = marshaller.pushMap(0);

    marshaller.push(Value(42));
    marshaller.putMapProperty(STRING_LITERAL("magicNumber"), objectIndex);
    ASSERT_TRUE(exceptionTracker);
    marshaller.push(Value(STRING_LITERAL("Simon")));
    marshaller.putMapProperty(STRING_LITERAL("name"), objectIndex);
    ASSERT_TRUE(exceptionTracker);

    auto success = marshaller.getMapProperty(STRING_LITERAL("name"), objectIndex);
    ASSERT_TRUE(success);
    auto value = marshaller.getString(-1);
    marshaller.pop();
    ASSERT_EQ(STRING_LITERAL("Simon"), value);

    ASSERT_EQ(1, marshaller.size());

    success = marshaller.getMapProperty(STRING_LITERAL("UnknownKey"), objectIndex);
    ASSERT_FALSE(success);

    success = marshaller.getMapProperty(STRING_LITERAL("magicNumber"), objectIndex);
    ASSERT_TRUE(success);

    auto newValue = marshaller.getInt(-1);
    marshaller.pop();
    ASSERT_EQ(42, newValue);

    ASSERT_EQ(1, marshaller.size());
}

TEST(Marshaller, canGetMapPropertyWithFlags) {
    SimpleExceptionTracker exceptionTracker;
    Marshaller marshaller(exceptionTracker);

    auto objectIndex = marshaller.pushMap(0);

    marshaller.push(Value::undefined());
    marshaller.putMapProperty(STRING_LITERAL("firstname"), objectIndex);
    ASSERT_TRUE(exceptionTracker);

    marshaller.push(Value());
    marshaller.putMapProperty(STRING_LITERAL("lastname"), objectIndex);
    ASSERT_TRUE(exceptionTracker);

    ASSERT_TRUE(
        marshaller.getMapProperty(STRING_LITERAL("firstname"), objectIndex, Marshaller::GetMapPropertyFlags::None));
    marshaller.pop();

    ASSERT_TRUE(
        marshaller.getMapProperty(STRING_LITERAL("lastname"), objectIndex, Marshaller::GetMapPropertyFlags::None));
    marshaller.pop();

    ASSERT_FALSE(marshaller.getMapProperty(
        STRING_LITERAL("firstname"), objectIndex, Marshaller::GetMapPropertyFlags::IgnoreNullOrUndefined));
    ASSERT_FALSE(marshaller.getMapProperty(
        STRING_LITERAL("lastname"), objectIndex, Marshaller::GetMapPropertyFlags::IgnoreNullOrUndefined));

    ASSERT_TRUE(marshaller.getMapProperty(
        STRING_LITERAL("thisDoesntExist"), objectIndex, Marshaller::GetMapPropertyFlags::PushUndefinedOnMiss));

    ASSERT_EQ(Value::undefined(), marshaller.get(-1));
}

TEST(Marshaller, canIterateOverMap) {
    SimpleExceptionTracker exceptionTracker;
    Marshaller marshaller(exceptionTracker);

    auto objectIndex = marshaller.pushMap(0);

    marshaller.push(Value(42));
    marshaller.putMapProperty(STRING_LITERAL("magicNumber"), objectIndex);
    ASSERT_TRUE(exceptionTracker);
    marshaller.push(Value(STRING_LITERAL("Simon")));
    marshaller.putMapProperty(STRING_LITERAL("name"), objectIndex);
    ASSERT_TRUE(exceptionTracker);

    auto iteratorIndex = marshaller.pushMapIterator(objectIndex);
    ASSERT_TRUE(exceptionTracker);

    std::vector<StringBox> keys;
    ValueArrayBuilder values;

    while (marshaller.pushMapIteratorNextEntry(iteratorIndex)) {
        auto key = marshaller.get(-2);
        ASSERT_TRUE(key.isString());

        keys.emplace_back(key.toStringBox());
        values.emplace(marshaller.get(-1));

        ASSERT_TRUE(exceptionTracker);
        marshaller.pop();
        ASSERT_TRUE(exceptionTracker);
        marshaller.pop();
        ASSERT_TRUE(exceptionTracker);
    }

    auto allValues = values.build();

    marshaller.pop();
    ASSERT_TRUE(exceptionTracker);

    ASSERT_EQ(static_cast<size_t>(2), keys.size());
    ASSERT_EQ(1, marshaller.size());

    auto expectedMap = makeShared<ValueMap>();
    (*expectedMap)[STRING_LITERAL("magicNumber")] = Value(42);
    (*expectedMap)[STRING_LITERAL("name")] = Value(STRING_LITERAL("Simon"));

    auto retrievedMap = makeShared<ValueMap>();
    (*retrievedMap)[keys[0]] = (*allValues)[0];
    (*retrievedMap)[keys[1]] = (*allValues)[1];

    ASSERT_EQ(*expectedMap, *retrievedMap);
}

TEST(Marshaller, canBuildArray) {
    SimpleExceptionTracker exceptionTracker;
    Marshaller marshaller(exceptionTracker);

    auto objectIndex = marshaller.pushArray(3);

    marshaller.push(Value(1));
    marshaller.setArrayItem(objectIndex, 0);
    ASSERT_TRUE(exceptionTracker);
    marshaller.push(Value(2));
    marshaller.setArrayItem(objectIndex, 1);
    ASSERT_TRUE(exceptionTracker);
    marshaller.push(Value(3));
    marshaller.setArrayItem(objectIndex, 2);
    ASSERT_TRUE(exceptionTracker);

    ASSERT_EQ(1, marshaller.size());

    auto array = marshaller.getArray(0);
    ASSERT_NE(nullptr, array);

    ValueArrayBuilder expected;
    expected.append(Value(1));
    expected.append(Value(2));
    expected.append(Value(3));

    ASSERT_EQ(*expected.build(), *array);
}

TEST(Marshaller, canIterateOnArray) {
    SimpleExceptionTracker exceptionTracker;
    Marshaller marshaller(exceptionTracker);

    auto objectIndex = marshaller.pushArray(3);

    marshaller.push(Value(1));
    marshaller.setArrayItem(objectIndex, 0);
    ASSERT_TRUE(exceptionTracker);
    marshaller.push(Value(2));
    marshaller.setArrayItem(objectIndex, 1);
    ASSERT_TRUE(exceptionTracker);
    marshaller.push(Value(3));
    marshaller.setArrayItem(objectIndex, 2);
    ASSERT_TRUE(exceptionTracker);

    auto arrayLength = marshaller.getArrayLength(objectIndex);
    ASSERT_EQ(3, arrayLength);

    marshaller.getArrayItem(objectIndex, 0);
    ASSERT_TRUE(exceptionTracker);
    ASSERT_EQ(1, marshaller.getInt(-1));
    marshaller.pop();
    ASSERT_TRUE(exceptionTracker);

    marshaller.getArrayItem(objectIndex, 1);
    ASSERT_TRUE(exceptionTracker);
    ASSERT_EQ(2, marshaller.getInt(-1));
    marshaller.pop();
    ASSERT_TRUE(exceptionTracker);

    marshaller.getArrayItem(objectIndex, 2);
    ASSERT_TRUE(exceptionTracker);
    ASSERT_EQ(3, marshaller.getInt(-1));
    marshaller.pop();
    ASSERT_TRUE(exceptionTracker);

    ASSERT_EQ(1, marshaller.size());
}

TEST(Marshaller, canGetItemArrayByRandomIndex) {
    SimpleExceptionTracker exceptionTracker;
    Marshaller marshaller(exceptionTracker);

    auto objectIndex = marshaller.pushArray(3);

    marshaller.push(Value(1));
    marshaller.setArrayItem(objectIndex, 0);
    ASSERT_TRUE(exceptionTracker);
    marshaller.push(Value(2));
    marshaller.setArrayItem(objectIndex, 1);
    ASSERT_TRUE(exceptionTracker);
    marshaller.push(Value(3));
    marshaller.setArrayItem(objectIndex, 2);
    ASSERT_TRUE(exceptionTracker);

    marshaller.getArrayItem(objectIndex, 1);
    ASSERT_EQ(2, marshaller.getInt(-1));
    marshaller.pop();
    ASSERT_TRUE(exceptionTracker);

    ASSERT_EQ(1, marshaller.size());
}

TEST(Marshaller, canCompare) {
    SimpleExceptionTracker exceptionTracker;
    Marshaller marshaller(exceptionTracker);

    marshaller.push(Value(1));
    marshaller.push(Value(2));
    marshaller.push(Value(3));

    Marshaller other(exceptionTracker);

    ASSERT_NE(marshaller, other);

    other.push(Value(1));
    other.push(Value(3));
    other.push(Value(2));

    ASSERT_NE(marshaller, other);

    other.popCount(2);
    other.push(Value(2));
    other.push(Value(3));

    ASSERT_EQ(marshaller, other);
}

TEST(Marshaller, canCopyMap) {
    SimpleExceptionTracker exceptionTracker;
    Marshaller marshaller(exceptionTracker);

    auto originalIndex = marshaller.pushMap(0);

    marshaller.push(Value(42));
    marshaller.putMapProperty(STRING_LITERAL("magicNumber"), originalIndex);
    ASSERT_TRUE(exceptionTracker);

    marshaller.push(Value(STRING_LITERAL("Simon")));
    marshaller.putMapProperty(STRING_LITERAL("name"), originalIndex);
    ASSERT_TRUE(exceptionTracker);

    auto copyIndex = marshaller.pushMap(0);
    marshaller.copyMap(originalIndex, copyIndex);
    ASSERT_TRUE(exceptionTracker);

    auto originalMap = marshaller.getMap(originalIndex);
    ASSERT_TRUE(exceptionTracker);
    auto copyMap = marshaller.getMap(copyIndex);
    ASSERT_TRUE(exceptionTracker);

    // References should be different
    ASSERT_NE(originalMap, copyMap);
    // Content should be the same
    ASSERT_EQ(*originalMap, *copyMap);
}

TEST(Marshaller, canMergeMaps) {
    SimpleExceptionTracker exceptionTracker;
    Marshaller marshaller(exceptionTracker);

    auto leftIndex = marshaller.pushMap(0);

    marshaller.push(Value(42));
    marshaller.putMapProperty(STRING_LITERAL("magicNumber"), leftIndex);
    ASSERT_TRUE(exceptionTracker);

    auto rightIndex = marshaller.pushMap(0);
    marshaller.push(Value(STRING_LITERAL("Simon")));
    marshaller.putMapProperty(STRING_LITERAL("name"), rightIndex);
    ASSERT_TRUE(exceptionTracker);

    auto mergedIndex = marshaller.mergeMaps(leftIndex, rightIndex);
    ASSERT_TRUE(exceptionTracker);

    auto mergedMap = marshaller.getMap(mergedIndex);
    ASSERT_TRUE(exceptionTracker);

    auto expectedObject = makeShared<ValueMap>();
    (*expectedObject)[STRING_LITERAL("magicNumber")] = Value(42);
    (*expectedObject)[STRING_LITERAL("name")] = Value(STRING_LITERAL("Simon"));

    ASSERT_EQ(*expectedObject, *mergedMap);
}

TEST(Marshaller, canSwap) {
    SimpleExceptionTracker exceptionTracker;
    Marshaller marshaller(exceptionTracker);

    auto leftIndex = marshaller.push(Value(1));
    auto rightIndex = marshaller.push(Value(2));

    ASSERT_EQ(Value(1), marshaller.get(leftIndex));
    ASSERT_EQ(Value(2), marshaller.get(rightIndex));

    marshaller.swap(leftIndex, rightIndex);
    ASSERT_TRUE(exceptionTracker);

    ASSERT_EQ(Value(2), marshaller.get(leftIndex));
    ASSERT_EQ(Value(1), marshaller.get(rightIndex));
}

} // namespace ValdiTest
