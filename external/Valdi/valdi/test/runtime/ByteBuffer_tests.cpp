#include "valdi_core/cpp/Utils/ByteBuffer.hpp"
#include <gtest/gtest.h>

using namespace Valdi;

namespace ValdiTest {

TEST(ByteBuffer, reallocatesOnAppend) {
    ByteBuffer buffer;

    ASSERT_EQ(static_cast<size_t>(0), buffer.size());
    ASSERT_EQ(static_cast<size_t>(0), buffer.capacity());

    buffer.appendWritable(1);

    ASSERT_EQ(static_cast<size_t>(1), buffer.size());
    ASSERT_EQ(static_cast<size_t>(sizeof(size_t)), buffer.capacity());
}

TEST(ByteBuffer, reallocatesByPowerOfTwo) {
    ByteBuffer buffer;

    buffer.appendWritable(9);

    ASSERT_EQ(static_cast<size_t>(9), buffer.size());
    ASSERT_EQ(static_cast<size_t>(16), buffer.capacity());
}

TEST(ByteBuffer, copiesOnReallocate) {
    ByteBuffer buffer;

    auto* data = buffer.appendWritable(sizeof(size_t));

    ASSERT_EQ(sizeof(size_t), buffer.size());
    ASSERT_EQ(sizeof(size_t), buffer.capacity());
    ASSERT_EQ(data, buffer.data());

    *reinterpret_cast<size_t*>(data) = 133742121212;

    ASSERT_EQ(static_cast<size_t>(133742121212), *reinterpret_cast<size_t*>(buffer.data()));

    buffer.appendWritable(sizeof(size_t));

    ASSERT_EQ(sizeof(size_t) * 2, buffer.size());
    ASSERT_EQ(sizeof(size_t) * 2, buffer.capacity());
    ASSERT_NE(data, buffer.data());

    ASSERT_EQ(static_cast<size_t>(133742121212), *reinterpret_cast<size_t*>(buffer.data()));
}

TEST(ByteBuffer, returnsWritablePointerOnResize) {
    ByteBuffer buffer;

    auto* data = buffer.appendWritable(sizeof(size_t));

    ASSERT_EQ(sizeof(size_t), buffer.size());
    ASSERT_EQ(sizeof(size_t), buffer.capacity());
    ASSERT_EQ(data, buffer.data());

    *reinterpret_cast<size_t*>(data) = 133742121212;

    ASSERT_EQ(static_cast<size_t>(133742121212), *reinterpret_cast<size_t*>(buffer.data()));

    auto* newData = buffer.appendWritable(sizeof(size_t));
    *reinterpret_cast<size_t*>(newData) = 17;

    ASSERT_EQ(sizeof(size_t) * 2, buffer.size());
    ASSERT_EQ(sizeof(size_t) * 2, buffer.capacity());

    ASSERT_EQ(static_cast<size_t>(133742121212), *reinterpret_cast<size_t*>(buffer[0]));
    ASSERT_EQ(static_cast<size_t>(17), *reinterpret_cast<size_t*>(buffer[sizeof(size_t)]));
}

TEST(ByteBuffer, canBeCompared) {
    ByteBuffer buffer;

    auto* myInt = reinterpret_cast<size_t*>(buffer.appendWritable(sizeof(size_t)));
    *myInt = 42;

    ByteBuffer buffer2;

    auto* myInt2 = reinterpret_cast<size_t*>(buffer2.appendWritable(sizeof(size_t)));
    *myInt2 = 42;

    ASSERT_EQ(buffer, buffer2);

    ByteBuffer buffer3;
    auto* myInt3 = reinterpret_cast<size_t*>(buffer3.appendWritable(sizeof(size_t)));
    *myInt3 = 43;

    ASSERT_NE(buffer, buffer3);
}

TEST(ByteBuffer, canBeCopied) {
    ByteBuffer buffer;

    auto* myInt = reinterpret_cast<size_t*>(buffer.appendWritable(sizeof(size_t)));
    *myInt = 42;

    ByteBuffer copyOperator;
    copyOperator = buffer;

    ASSERT_NE(buffer.data(), copyOperator.data());
    ASSERT_EQ(buffer.size(), copyOperator.size());
    ASSERT_EQ(buffer, copyOperator);

    ByteBuffer copyConstructor(buffer);

    ASSERT_NE(buffer.data(), copyOperator.data());
    ASSERT_EQ(buffer.size(), copyOperator.size());
    ASSERT_EQ(buffer, copyOperator);
}

TEST(ByteBuffer, canBeMoved) {
    ByteBuffer buffer;

    auto* myInt = reinterpret_cast<size_t*>(buffer.appendWritable(sizeof(size_t)));
    *myInt = 42;

    ByteBuffer moveOperator;
    moveOperator = std::move(buffer);

    ASSERT_EQ(static_cast<size_t>(0), buffer.size());
    ASSERT_EQ(static_cast<size_t>(0), buffer.capacity());
    ASSERT_EQ(nullptr, buffer.data());

    ASSERT_EQ(sizeof(size_t), moveOperator.size());
    ASSERT_EQ(sizeof(size_t), moveOperator.capacity());
    ASSERT_EQ(myInt, reinterpret_cast<size_t*>(moveOperator.data()));

    ByteBuffer moveConstructor(std::move(moveOperator));

    ASSERT_EQ(static_cast<size_t>(0), moveOperator.size());
    ASSERT_EQ(static_cast<size_t>(0), moveOperator.capacity());
    ASSERT_EQ(nullptr, moveOperator.data());

    ASSERT_EQ(sizeof(size_t), moveConstructor.size());
    ASSERT_EQ(sizeof(size_t), moveConstructor.capacity());
    ASSERT_EQ(myInt, reinterpret_cast<size_t*>(moveConstructor.data()));
}

TEST(ByteBuffer, canAppendAndWrite) {
    ByteBuffer buffer;

    std::string str1 = "Hello";
    std::string str2 = " ";
    std::string str3 = "World!";

    buffer.append(reinterpret_cast<const Byte*>(str1.c_str()),
                  reinterpret_cast<const Byte*>(str1.c_str() + str1.length()));

    ASSERT_EQ(static_cast<size_t>(5), buffer.size());
    ASSERT_EQ(static_cast<size_t>(sizeof(size_t)), buffer.capacity());

    buffer.append(reinterpret_cast<const Byte*>(str2.c_str()),
                  reinterpret_cast<const Byte*>(str2.c_str() + str2.length()));

    ASSERT_EQ(static_cast<size_t>(6), buffer.size());
    ASSERT_EQ(static_cast<size_t>(sizeof(size_t)), buffer.capacity());

    buffer.append(reinterpret_cast<const Byte*>(str3.c_str()),
                  reinterpret_cast<const Byte*>(str3.c_str() + str3.length()));

    ASSERT_EQ(static_cast<size_t>(12), buffer.size());
    ASSERT_EQ(static_cast<size_t>(sizeof(size_t) * 2), buffer.capacity());

    auto result = std::string(reinterpret_cast<const char*>(buffer.data()), buffer.size());
    ASSERT_EQ("Hello World!", result);
}

TEST(ByteBuffer, canShift) {
    ByteBuffer buffer;

    std::string str1 = "Hello World!";
    buffer.append(reinterpret_cast<const Byte*>(str1.c_str()),
                  reinterpret_cast<const Byte*>(str1.c_str() + str1.length()));

    auto result = std::string(reinterpret_cast<const char*>(buffer.data()), buffer.size());
    ASSERT_EQ("Hello World!", result);
    ASSERT_EQ(static_cast<size_t>(12), buffer.size());

    buffer.shift(1);

    result = std::string(reinterpret_cast<const char*>(buffer.data()), buffer.size());
    ASSERT_EQ("ello World!", result);
    ASSERT_EQ(static_cast<size_t>(11), buffer.size());

    buffer.shift(5);

    result = std::string(reinterpret_cast<const char*>(buffer.data()), buffer.size());
    ASSERT_EQ("World!", result);
    ASSERT_EQ(static_cast<size_t>(6), buffer.size());
}

TEST(ByteBuffer, canBeSet) {
    ByteBuffer buffer;

    std::string str1 = "Hello World!";

    buffer.set(reinterpret_cast<const Byte*>(str1.c_str()),
               reinterpret_cast<const Byte*>(str1.c_str() + str1.length()));
    auto result = std::string(reinterpret_cast<const char*>(buffer.data()), buffer.size());

    ASSERT_EQ("Hello World!", result);
    ASSERT_EQ(static_cast<size_t>(12), buffer.size());
    ASSERT_EQ(static_cast<size_t>(12), buffer.capacity());

    std::string str2 = "World!";

    buffer.set(reinterpret_cast<const Byte*>(str2.c_str()),
               reinterpret_cast<const Byte*>(str2.c_str() + str2.length()));

    result = std::string(reinterpret_cast<const char*>(buffer.data()), buffer.size());

    ASSERT_EQ("World!", result);
    ASSERT_EQ(static_cast<size_t>(6), buffer.size());
    ASSERT_EQ(static_cast<size_t>(6), buffer.capacity());

    buffer.set(reinterpret_cast<const Byte*>(str2.c_str()), reinterpret_cast<const Byte*>(str2.c_str()));

    ASSERT_EQ(static_cast<size_t>(0), buffer.size());
    ASSERT_EQ(static_cast<size_t>(0), buffer.capacity());
    ASSERT_EQ(nullptr, buffer.data());
}

TEST(ByteBuffer, canResize) {
    ByteBuffer buffer;

    buffer.resize(5);

    ASSERT_EQ(static_cast<size_t>(5), buffer.size());
    ASSERT_EQ(static_cast<size_t>(5), buffer.capacity());

    strncpy(reinterpret_cast<char*>(buffer.data()), "Hello", 5);

    auto result = std::string(reinterpret_cast<const char*>(buffer.data()), buffer.size());

    ASSERT_EQ("Hello", result);

    buffer.resize(2);

    result = std::string(reinterpret_cast<const char*>(buffer.data()), buffer.size());

    ASSERT_EQ("He", result);

    buffer.resize(12);

    ASSERT_EQ(static_cast<size_t>(12), buffer.size());
    ASSERT_EQ(static_cast<size_t>(12), buffer.capacity());

    strncpy(reinterpret_cast<char*>(buffer.data()), "Hello World!", 12);

    result = std::string(reinterpret_cast<const char*>(buffer.data()), buffer.size());

    ASSERT_EQ("Hello World!", result);
}

TEST(ByteBuffer, canConvertToBytesVec) {
    ByteBuffer buffer;

    std::string str1 = "Hello World!";

    buffer.set(reinterpret_cast<const Byte*>(str1.c_str()),
               reinterpret_cast<const Byte*>(str1.c_str() + str1.length()));

    auto bytesVec = buffer.toBytesVec();

    for (size_t i = 0; i < buffer.size(); i++) {
        ASSERT_EQ(*buffer[i], bytesVec[i]);
    }
}

TEST(ByteBuffer, canAppendByteByByte) {
    ByteBuffer buffer;

    buffer.append('H');
    buffer.append('e');
    buffer.append('l');
    buffer.append('l');
    buffer.append('o');

    ASSERT_EQ(static_cast<size_t>(5), buffer.size());
    auto result = std::string(reinterpret_cast<const char*>(buffer.data()), buffer.size());

    ASSERT_EQ("Hello", result);
}

TEST(Bytes, canStealExternalBuffer) {
    Bytes bytes;

    std::vector<Byte> buffer;
    buffer.assign({0, 1, 2, 3});

    ASSERT_EQ(static_cast<size_t>(0), bytes.size());
    ASSERT_EQ(static_cast<size_t>(4), buffer.size());
    ASSERT_EQ(nullptr, bytes.data());
    ASSERT_NE(nullptr, buffer.data());

    bytes.assignVec(std::move(buffer));

    ASSERT_EQ(static_cast<size_t>(4), bytes.size());
    ASSERT_EQ(static_cast<size_t>(0), buffer.size());
    ASSERT_NE(nullptr, bytes.data());
    ASSERT_EQ(nullptr, buffer.data());

    ASSERT_EQ(0, bytes[0]);
    ASSERT_EQ(1, bytes[1]);
    ASSERT_EQ(2, bytes[2]);
    ASSERT_EQ(3, bytes[3]);
}

} // namespace ValdiTest
