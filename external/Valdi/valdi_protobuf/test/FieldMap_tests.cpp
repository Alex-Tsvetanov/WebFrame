#include "protogen/test.pb.h"
#include "valdi_protobuf/FieldMap.hpp"
#include "valdi_protobuf/RepeatedField.hpp"
#include "gtest/gtest.h"

using namespace Valdi;
namespace {

TEST(FieldMap, canInsertInVec) {
    auto str = StaticString::makeUTF8("Hello World!");
    Protobuf::FieldMap fieldMap;
    fieldMap[2] = Protobuf::Field::string(str);
    fieldMap[7].setFloat(42.0f);
    fieldMap[12].setInt32(1000);

    ASSERT_FALSE(fieldMap.isUsingMap());

    auto entries = fieldMap.getEntries();
    ASSERT_EQ(static_cast<size_t>(3), entries.size());

    ASSERT_EQ(static_cast<size_t>(2), entries[0].number);
    ASSERT_EQ(str.get(), entries[0].value->getString());
    ASSERT_EQ(static_cast<size_t>(7), entries[1].number);
    ASSERT_EQ(42.0f, entries[1].value->getFloat());
    ASSERT_EQ(static_cast<size_t>(12), entries[2].number);
    ASSERT_EQ(1000, entries[2].value->getInt32());
}

TEST(FieldMap, canFindInVec) {
    auto str = StaticString::makeUTF8("Hello World!");
    Protobuf::FieldMap fieldMap;
    fieldMap[2] = Protobuf::Field::string(str);
    fieldMap[7].setFloat(42.0f);
    fieldMap[12].setInt32(1000);

    ASSERT_FALSE(fieldMap.isUsingMap());

    const auto* it = fieldMap.find(0);
    ASSERT_TRUE(it == nullptr);

    it = fieldMap.find(1);
    ASSERT_TRUE(it == nullptr);

    it = fieldMap.find(2);
    ASSERT_TRUE(it != nullptr);
    ASSERT_EQ(str.get(), it->getString());

    it = fieldMap.find(3);
    ASSERT_TRUE(it == nullptr);

    it = fieldMap.find(7);
    ASSERT_TRUE(it != nullptr);
    ASSERT_EQ(42.0f, it->getFloat());

    it = fieldMap.find(8);
    ASSERT_TRUE(it == nullptr);

    it = fieldMap.find(12);
    ASSERT_TRUE(it != nullptr);
    ASSERT_EQ(1000, it->getInt32());

    it = fieldMap.find(13);
    ASSERT_TRUE(it == nullptr);
}

TEST(FieldMap, canInsertInMap) {
    auto str = StaticString::makeUTF8("Hello World!");
    Protobuf::FieldMap fieldMap;
    fieldMap[65] = Protobuf::Field::string(str);
    fieldMap[7].setFloat(42.0f);
    fieldMap[12].setInt32(1000);

    ASSERT_TRUE(fieldMap.isUsingMap());

    auto entries = fieldMap.getEntries();
    ASSERT_EQ(static_cast<size_t>(3), entries.size());

    ASSERT_EQ(static_cast<size_t>(7), entries[0].number);
    ASSERT_EQ(42.0f, entries[0].value->getFloat());
    ASSERT_EQ(static_cast<size_t>(12), entries[1].number);
    ASSERT_EQ(1000, entries[1].value->getInt32());

    ASSERT_EQ(static_cast<size_t>(65), entries[2].number);
    ASSERT_EQ(str.get(), entries[2].value->getString());
}

TEST(FieldMap, canFindInMap) {
    auto str = StaticString::makeUTF8("Hello World!");
    Protobuf::FieldMap fieldMap;
    fieldMap[65] = Protobuf::Field::string(str);
    fieldMap[7].setFloat(42.0f);
    fieldMap[12].setInt32(1000);

    ASSERT_TRUE(fieldMap.isUsingMap());

    const auto* it = fieldMap.find(0);
    ASSERT_TRUE(it == nullptr);

    it = fieldMap.find(1);
    ASSERT_TRUE(it == nullptr);

    it = fieldMap.find(7);
    ASSERT_TRUE(it != nullptr);
    ASSERT_EQ(42.0f, it->getFloat());

    it = fieldMap.find(8);
    ASSERT_TRUE(it == nullptr);

    it = fieldMap.find(12);
    ASSERT_TRUE(it != nullptr);
    ASSERT_EQ(1000, it->getInt32());

    it = fieldMap.find(13);
    ASSERT_TRUE(it == nullptr);

    it = fieldMap.find(65);
    ASSERT_TRUE(it != nullptr);
    ASSERT_EQ(str.get(), it->getString());

    it = fieldMap.find(66);
    ASSERT_TRUE(it == nullptr);
}

TEST(FieldMap, switchesFromVecToMapWhenIndexTooLarge) {
    auto str = StaticString::makeUTF8("Hello World!");
    Protobuf::FieldMap fieldMap;
    fieldMap[2] = Protobuf::Field::string(str);
    fieldMap[7].setFloat(42.0f);

    ASSERT_FALSE(fieldMap.isUsingMap());

    fieldMap[70].setInt32(1000);

    ASSERT_TRUE(fieldMap.isUsingMap());

    auto entries = fieldMap.getEntries();
    ASSERT_EQ(static_cast<size_t>(3), entries.size());

    ASSERT_EQ(static_cast<size_t>(2), entries[0].number);
    ASSERT_EQ(str.get(), entries[0].value->getString());

    ASSERT_EQ(static_cast<size_t>(7), entries[1].number);
    ASSERT_EQ(42.0f, entries[1].value->getFloat());
    ASSERT_EQ(static_cast<size_t>(70), entries[2].number);
    ASSERT_EQ(1000, entries[2].value->getInt32());
}

TEST(FieldMap, canCopyVec) {
    auto str = StaticString::makeUTF8("Hello World!");
    Protobuf::FieldMap fieldMap;
    fieldMap[2] = Protobuf::Field::string(str);

    ASSERT_EQ(2, str.use_count());
    ASSERT_FALSE(fieldMap.isUsingMap());

    {
        Protobuf::FieldMap copy(fieldMap);

        ASSERT_EQ(3, str.use_count());
        ASSERT_FALSE(copy.isUsingMap());

        auto entries = copy.getEntries();
        ASSERT_EQ(static_cast<size_t>(1), entries.size());

        ASSERT_EQ(static_cast<size_t>(2), entries[0].number);
        ASSERT_EQ(str.get(), entries[0].value->getString());

        fieldMap.clear();
        ASSERT_EQ(2, str.use_count());
        fieldMap[7] = Protobuf::Field::string(str);
        ASSERT_EQ(3, str.use_count());
        fieldMap[10].setInt32(42);

        copy = fieldMap;
        ASSERT_EQ(3, str.use_count());

        entries = copy.getEntries();
        ASSERT_EQ(static_cast<size_t>(2), entries.size());

        ASSERT_EQ(static_cast<size_t>(7), entries[0].number);
        ASSERT_EQ(str.get(), entries[0].value->getString());
        ASSERT_EQ(static_cast<size_t>(10), entries[1].number);
        ASSERT_EQ(42, entries[1].value->getInt32());
    }

    ASSERT_EQ(2, str.use_count());
}

TEST(FieldMap, canCopyMap) {
    auto str = StaticString::makeUTF8("Hello World!");
    Protobuf::FieldMap fieldMap;
    fieldMap[70] = Protobuf::Field::string(str);

    ASSERT_EQ(2, str.use_count());
    ASSERT_TRUE(fieldMap.isUsingMap());

    {
        Protobuf::FieldMap copy(fieldMap);

        ASSERT_EQ(3, str.use_count());
        ASSERT_TRUE(copy.isUsingMap());

        auto entries = copy.getEntries();
        ASSERT_EQ(static_cast<size_t>(1), entries.size());

        ASSERT_EQ(static_cast<size_t>(70), entries[0].number);
        ASSERT_EQ(str.get(), entries[0].value->getString());

        fieldMap.clear();
        ASSERT_EQ(2, str.use_count());
        fieldMap[65] = Protobuf::Field::string(str);
        ASSERT_EQ(3, str.use_count());
        fieldMap[10].setInt32(42);

        copy = fieldMap;
        ASSERT_EQ(3, str.use_count());

        entries = copy.getEntries();
        ASSERT_EQ(static_cast<size_t>(2), entries.size());

        ASSERT_EQ(static_cast<size_t>(10), entries[0].number);
        ASSERT_EQ(42, entries[0].value->getInt32());
        ASSERT_EQ(static_cast<size_t>(65), entries[1].number);
        ASSERT_EQ(str.get(), entries[1].value->getString());
    }

    ASSERT_EQ(2, str.use_count());
}

TEST(FieldMap, canMoveVec) {
    auto str = StaticString::makeUTF8("Hello World!");
    Protobuf::FieldMap fieldMap;
    fieldMap[2] = Protobuf::Field::string(str);

    ASSERT_EQ(2, str.use_count());
    ASSERT_FALSE(fieldMap.isUsingMap());

    {
        Protobuf::FieldMap copy(std::move(fieldMap));

        ASSERT_EQ(2, str.use_count());
        ASSERT_FALSE(copy.isUsingMap());

        auto entries = copy.getEntries();
        ASSERT_EQ(static_cast<size_t>(1), entries.size());

        ASSERT_EQ(static_cast<size_t>(2), entries[0].number);
        ASSERT_EQ(str.get(), entries[0].value->getString());

        fieldMap.clear();
        ASSERT_EQ(2, str.use_count());
        fieldMap[7] = Protobuf::Field::string(str);
        ASSERT_EQ(3, str.use_count());
        fieldMap[10].setInt32(42);

        copy = std::move(fieldMap);
        ASSERT_EQ(2, str.use_count());

        entries = copy.getEntries();
        ASSERT_EQ(static_cast<size_t>(2), entries.size());

        ASSERT_EQ(static_cast<size_t>(7), entries[0].number);
        ASSERT_EQ(str.get(), entries[0].value->getString());
        ASSERT_EQ(static_cast<size_t>(10), entries[1].number);
        ASSERT_EQ(42, entries[1].value->getInt32());
    }

    ASSERT_EQ(1, str.use_count());
}

TEST(FieldMap, canMoveMap) {
    auto str = StaticString::makeUTF8("Hello World!");
    Protobuf::FieldMap fieldMap;
    fieldMap[70] = Protobuf::Field::string(str);

    ASSERT_EQ(2, str.use_count());
    ASSERT_TRUE(fieldMap.isUsingMap());

    {
        Protobuf::FieldMap copy(std::move(fieldMap));

        ASSERT_EQ(2, str.use_count());
        ASSERT_TRUE(copy.isUsingMap());

        auto entries = copy.getEntries();
        ASSERT_EQ(static_cast<size_t>(1), entries.size());

        ASSERT_EQ(static_cast<size_t>(70), entries[0].number);
        ASSERT_EQ(str.get(), entries[0].value->getString());

        fieldMap.clear();
        ASSERT_EQ(2, str.use_count());
        fieldMap[65] = Protobuf::Field::string(str);
        ASSERT_EQ(3, str.use_count());
        fieldMap[10].setInt32(42);

        copy = std::move(fieldMap);
        ASSERT_EQ(2, str.use_count());

        entries = copy.getEntries();
        ASSERT_EQ(static_cast<size_t>(2), entries.size());

        ASSERT_EQ(static_cast<size_t>(10), entries[0].number);
        ASSERT_EQ(42, entries[0].value->getInt32());
        ASSERT_EQ(static_cast<size_t>(65), entries[1].number);
        ASSERT_EQ(str.get(), entries[1].value->getString());
    }

    ASSERT_EQ(1, str.use_count());
}

} // namespace
