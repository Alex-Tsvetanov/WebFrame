#include "protogen/test.pb.h"
#include "valdi_core/cpp/Utils/Function.hpp"
#include "valdi_protobuf/Field.hpp"
#include "valdi_protobuf/Message.hpp"
#include "valdi_protobuf/RepeatedField.hpp"
#include "gtest/gtest.h"
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/repeated_field.h> // IWYU pragma: export

using namespace Valdi;
namespace {

using WireFormat = google::protobuf::internal::WireFormatLite;
using CodedOutputStream = google::protobuf::io::CodedOutputStream;

template<typename T>
bool isFieldZero(T value) {
    const auto* ptr = reinterpret_cast<const Byte*>(&value);

    for (size_t i = 0; i < sizeof(T); i++) {
        if (ptr[i] != 0) {
            return false;
        }
    }

    return true;
}

template<typename CType, enum WireFormat::FieldType DeclaredType>
static std::optional<CType> readPrimitive(ByteBuffer& buffer) {
    google::protobuf::io::CodedInputStream inputStream(reinterpret_cast<uint8_t*>(buffer.data()), buffer.size());

    if (inputStream.ReadTag() == 0) {
        return std::nullopt;
    }

    CType output;

    if (!WireFormat::ReadPrimitive<CType, DeclaredType>(&inputStream, &output)) {
        return std::nullopt;
    }
    return {output};
}

class FieldTest : public ::testing::Test {
protected:
    Protobuf::Field field;

    template<enum WireFormat::FieldType DeclaredType, typename CType>
    void checkValue(ByteBuffer& buffer, CType valueToCompare) {
        auto read = readPrimitive<CType, DeclaredType>(buffer);
        ASSERT_TRUE(read);
        if (!read) {
            return;
        }

        ASSERT_EQ(read.value(), valueToCompare);
    }

    void checkVarintConformance(ByteBuffer& buffer) {
        // Check that the behavior of our field behaves constantly
        // with Protobuf WireFormat
        checkValue<WireFormat::TYPE_INT32>(buffer, field.getInt32());
        checkValue<WireFormat::TYPE_INT64>(buffer, field.getInt64());
        checkValue<WireFormat::TYPE_UINT32>(buffer, field.getUInt32());
        checkValue<WireFormat::TYPE_UINT64>(buffer, field.getUInt64());
        checkValue<WireFormat::TYPE_SINT32>(buffer, field.getSInt32());
        checkValue<WireFormat::TYPE_SINT64>(buffer, field.getSInt64());
        checkValue<WireFormat::TYPE_BOOL>(buffer, field.getBool());
        checkValue<WireFormat::TYPE_ENUM>(buffer, static_cast<int>(field.getEnum()));
    }

    void checkFixed64Conformance(ByteBuffer& buffer) {
        checkValue<WireFormat::TYPE_FIXED64>(buffer, field.getFixed64());
        checkValue<WireFormat::TYPE_SFIXED64>(buffer, field.getSFixed64());
        checkValue<WireFormat::TYPE_DOUBLE>(buffer, field.getDouble());
    }

    void checkFixed32Conformance(ByteBuffer& buffer) {
        checkValue<WireFormat::TYPE_FIXED32>(buffer, field.getFixed32());
        checkValue<WireFormat::TYPE_SFIXED32>(buffer, field.getSFixed32());
        checkValue<WireFormat::TYPE_FLOAT>(buffer, field.getFloat());
    }

    template<typename F>
    void testEncode(F&& doEncode) {
        std::string target;
        size_t writtenCount;
        {
            google::protobuf::io::StringOutputStream stream(&target);
            google::protobuf::io::CodedOutputStream outputStream(&stream, false);

            doEncode(1, &outputStream);

            writtenCount = static_cast<size_t>(outputStream.ByteCount());
        }

        auto byteSize = field.byteSize(1, false, false);

        ASSERT_EQ(writtenCount, byteSize);

        ByteBuffer buffer;
        buffer.resize(byteSize);

        auto* bufferEnd = field.write(1, false, false, buffer.begin(), buffer.end());

        ASSERT_EQ(buffer.end(), bufferEnd);

        ASSERT_EQ(std::string_view(target.data(), writtenCount), buffer.toStringView());

        if (writtenCount > 0) {
            switch (field.getInternalType()) {
                case Protobuf::Field::InternalType::Varint:
                    checkVarintConformance(buffer);
                    break;
                case Protobuf::Field::InternalType::Fixed64:
                    checkFixed64Conformance(buffer);
                    break;
                case Protobuf::Field::InternalType::Fixed32:
                    checkFixed32Conformance(buffer);
                    break;
                default:
                    break;
            }
        }
    }

    template<typename T>
    void testField(T fieldValue,
                   void (Protobuf::Field::*setter)(T),
                   T (Protobuf::Field::*getter)() const,
                   void (*pbWriter)(int, T, google::protobuf::io::CodedOutputStream*)) {
        (field.*setter)(fieldValue);

        auto result = (field.*getter)();

        ASSERT_EQ(fieldValue, result);

        testEncode([&](int fieldIndex, google::protobuf::io::CodedOutputStream* outputStream) {
            if (!isFieldZero(fieldValue)) {
                pbWriter(fieldIndex, fieldValue, outputStream);
            }
        });
    }
};

TEST_F(FieldTest, supportsInt32) {
    testField(0, &Protobuf::Field::setInt32, &Protobuf::Field::getInt32, &WireFormat::WriteInt32);
    testField(42, &Protobuf::Field::setInt32, &Protobuf::Field::getInt32, &WireFormat::WriteInt32);
    testField(-42, &Protobuf::Field::setInt32, &Protobuf::Field::getInt32, &WireFormat::WriteInt32);
}

TEST_F(FieldTest, supportsSInt32) {
    testField(0, &Protobuf::Field::setSInt32, &Protobuf::Field::getSInt32, &WireFormat::WriteSInt32);
    testField(42, &Protobuf::Field::setSInt32, &Protobuf::Field::getSInt32, &WireFormat::WriteSInt32);
    testField(-42, &Protobuf::Field::setSInt32, &Protobuf::Field::getSInt32, &WireFormat::WriteSInt32);
}

TEST_F(FieldTest, supportsInt64) {
    testField(
        static_cast<int64_t>(0L), &Protobuf::Field::setInt64, &Protobuf::Field::getInt64, &WireFormat::WriteInt64);
    testField(static_cast<int64_t>(4200000000000000L),
              &Protobuf::Field::setInt64,
              &Protobuf::Field::getInt64,
              &WireFormat::WriteInt64);
    testField(static_cast<int64_t>(-4200000000000000L),
              &Protobuf::Field::setInt64,
              &Protobuf::Field::getInt64,
              &WireFormat::WriteInt64);
}

TEST_F(FieldTest, supportsSInt64) {
    testField(
        static_cast<int64_t>(0L), &Protobuf::Field::setSInt64, &Protobuf::Field::getSInt64, &WireFormat::WriteSInt64);
    testField(static_cast<int64_t>(4200000000000000L),
              &Protobuf::Field::setSInt64,
              &Protobuf::Field::getSInt64,
              &WireFormat::WriteSInt64);
    testField(static_cast<int64_t>(-4200000000000000L),
              &Protobuf::Field::setSInt64,
              &Protobuf::Field::getSInt64,
              &WireFormat::WriteSInt64);
}

TEST_F(FieldTest, supportsUInt32) {
    testField(
        static_cast<uint32_t>(0L), &Protobuf::Field::setUInt32, &Protobuf::Field::getUInt32, &WireFormat::WriteUInt32);
    testField(
        static_cast<uint32_t>(42u), &Protobuf::Field::setUInt32, &Protobuf::Field::getUInt32, &WireFormat::WriteUInt32);
    testField(static_cast<uint32_t>(3147483647u),
              &Protobuf::Field::setUInt32,
              &Protobuf::Field::getUInt32,
              &WireFormat::WriteUInt32);
}

TEST_F(FieldTest, supportsUInt64) {
    testField(
        static_cast<uint64_t>(0L), &Protobuf::Field::setUInt64, &Protobuf::Field::getUInt64, &WireFormat::WriteUInt64);
    testField(static_cast<uint64_t>(4200000000000000ul),
              &Protobuf::Field::setUInt64,
              &Protobuf::Field::getUInt64,
              &WireFormat::WriteUInt64);
    testField(static_cast<uint64_t>(15223372036854775807ul),
              &Protobuf::Field::setUInt64,
              &Protobuf::Field::getUInt64,
              &WireFormat::WriteUInt64);
}

TEST_F(FieldTest, supportsFloat) {
    testField(
        static_cast<float>(0.0f), &Protobuf::Field::setFloat, &Protobuf::Field::getFloat, &WireFormat::WriteFloat);
    testField(
        static_cast<float>(42.5f), &Protobuf::Field::setFloat, &Protobuf::Field::getFloat, &WireFormat::WriteFloat);
    testField(
        static_cast<float>(-42.5f), &Protobuf::Field::setFloat, &Protobuf::Field::getFloat, &WireFormat::WriteFloat);
}

TEST_F(FieldTest, supportsDouble) {
    testField(
        static_cast<double>(0.0f), &Protobuf::Field::setDouble, &Protobuf::Field::getDouble, &WireFormat::WriteDouble);
    testField(static_cast<double>(42.123456789f),
              &Protobuf::Field::setDouble,
              &Protobuf::Field::getDouble,
              &WireFormat::WriteDouble);
    testField(static_cast<double>(-42.123456789f),
              &Protobuf::Field::setDouble,
              &Protobuf::Field::getDouble,
              &WireFormat::WriteDouble);
}

TEST_F(FieldTest, supportsBool) {
    testField(static_cast<bool>(false), &Protobuf::Field::setBool, &Protobuf::Field::getBool, &WireFormat::WriteBool);
    testField(static_cast<bool>(true), &Protobuf::Field::setBool, &Protobuf::Field::getBool, &WireFormat::WriteBool);
}

TEST_F(FieldTest, supportsRaw) {
    std::string str = "Hello World";
    field.setRaw(Protobuf::Field::Raw(reinterpret_cast<const Byte*>(str.data()), str.length()));

    auto value = field.getRaw();

    ASSERT_EQ(reinterpret_cast<const Byte*>(str.data()), value.data);
    ASSERT_EQ(str.size(), value.length);

    testEncode([&](int fieldKey, google::protobuf::io::CodedOutputStream* outputStream) {
        WireFormat::WriteString(fieldKey, str, outputStream);
    });
}

TEST_F(FieldTest, supportsString) {
    auto str = StaticString::makeUTF8("Hello World");
    field.setString(str.get());

    ASSERT_EQ(2, str.use_count());

    const auto* str2 = field.getString();

    ASSERT_EQ(str.get(), str2);

    testEncode([&](int fieldKey, google::protobuf::io::CodedOutputStream* outputStream) {
        WireFormat::WriteString(fieldKey, str->toStdString(), outputStream);
    });

    auto emptyStr = StaticString::makeUTF8("");
    field.setString(emptyStr.get());

    ASSERT_EQ(1, str.use_count());
    ASSERT_EQ(2, emptyStr.use_count());

    testEncode([&](int fieldKey, google::protobuf::io::CodedOutputStream* outputStream) {});
}

TEST_F(FieldTest, supportsBytes) {
    auto bytes = makeShared<ByteBuffer>();
    bytes->append("Hello World");

    auto typedArray = makeShared<ValueTypedArray>(TypedArrayType::Uint8Array, bytes->toBytesView());

    field.setTypedArray(typedArray.get());

    ASSERT_EQ(2, typedArray.use_count());

    const auto* typedArrayPtr = field.getTypedArray();

    ASSERT_EQ(typedArray.get(), typedArrayPtr);

    testEncode([&](int fieldKey, google::protobuf::io::CodedOutputStream* outputStream) {
        WireFormat::WriteBytes(fieldKey, std::string(bytes->toStringView()), outputStream);
    });

    auto emptyBytes = makeShared<ValueTypedArray>(TypedArrayType::Uint8Array, makeShared<ByteBuffer>()->toBytesView());
    field.setTypedArray(emptyBytes.get());

    ASSERT_EQ(1, typedArray.use_count());
    ASSERT_EQ(2, emptyBytes.use_count());

    testEncode([&](int fieldKey, google::protobuf::io::CodedOutputStream* outputStream) {});
}

TEST_F(FieldTest, supportsRepeated) {
    testEncode([&](int fieldKey, google::protobuf::io::CodedOutputStream* outputStream) {});

    auto one = StaticString::makeUTF8("One");

    field.append().setString(one.get());

    testEncode([&](int fieldKey, google::protobuf::io::CodedOutputStream* outputStream) {
        WireFormat::WriteString(fieldKey, one->toStdString(), outputStream);
    });

    auto two = StaticString::makeUTF8("Two");
    auto three = StaticString::makeUTF8("Three");

    field.append().setString(two.get());
    field.append().setString(three.get());

    testEncode([&](int fieldKey, google::protobuf::io::CodedOutputStream* outputStream) {
        WireFormat::WriteString(fieldKey, one->toStdString(), outputStream);
        WireFormat::WriteString(fieldKey, two->toStdString(), outputStream);
        WireFormat::WriteString(fieldKey, three->toStdString(), outputStream);
    });
}

TEST_F(FieldTest, usesRegularRepeatedWhenEncodingMixedRepeatedField) {
    testEncode([&](int fieldKey, google::protobuf::io::CodedOutputStream* outputStream) {});

    auto str = StaticString::makeUTF8("Hello World");

    field.append().setInt32(42);
    field.append().setString(str.get());
    field.append().setInt32(100);

    testEncode([&](int fieldKey, google::protobuf::io::CodedOutputStream* outputStream) {
        WireFormat::WriteInt32(fieldKey, 42, outputStream);
        WireFormat::WriteString(fieldKey, str->toStdString(), outputStream);
        WireFormat::WriteInt32(fieldKey, 100, outputStream);
    });
}

template<typename T>
static std::string packRepeatedField(const google::protobuf::RepeatedField<T>& values,
                                     void (*doWrite)(google::protobuf::io::CodedOutputStream*, T)) {
    std::string output;
    google::protobuf::io::StringOutputStream outputStream(&output);
    google::protobuf::io::CodedOutputStream codedOutputStream(&outputStream);

    for (const auto& value : values) {
        doWrite(&codedOutputStream, value);
    }

    return output;
}

template<typename T>
static void writeRepeatedField(int fieldKey,
                               google::protobuf::io::CodedOutputStream* outputStream,
                               const google::protobuf::RepeatedField<T>& values,
                               void (*doWrite)(google::protobuf::io::CodedOutputStream*, T)) {
    auto packed = packRepeatedField(values, doWrite);
    WireFormat::WriteBytes(fieldKey, packed, outputStream);
}

static void writeInt32(google::protobuf::io::CodedOutputStream* outputStream, int32_t value) {
    outputStream->WriteVarint32(static_cast<uint32_t>(value));
}

static void writeFloat(google::protobuf::io::CodedOutputStream* outputStream, float value) {
    outputStream->WriteLittleEndian32(*reinterpret_cast<uint32_t*>(&value));
}

static void writeDouble(google::protobuf::io::CodedOutputStream* outputStream, double value) {
    outputStream->WriteLittleEndian64(*reinterpret_cast<uint64_t*>(&value));
}

TEST_F(FieldTest, supportsPackedVarintRepeated) {
    testEncode([&](int fieldKey, google::protobuf::io::CodedOutputStream* outputStream) {});

    field.append().setInt32(1);

    testEncode([&](int fieldKey, google::protobuf::io::CodedOutputStream* outputStream) {
        google::protobuf::RepeatedField<int32_t> values;
        values.Add(1);

        writeRepeatedField(fieldKey, outputStream, values, &writeInt32);
    });

    field.append().setInt32(2);
    field.append().setInt32(345678901);

    testEncode([&](int fieldKey, google::protobuf::io::CodedOutputStream* outputStream) {
        google::protobuf::RepeatedField<int32_t> values;
        values.Add(1);
        values.Add(2);
        values.Add(345678901);

        writeRepeatedField(fieldKey, outputStream, values, &writeInt32);
    });
}

TEST_F(FieldTest, supportsPackedFixed32Repeated) {
    testEncode([&](int fieldKey, google::protobuf::io::CodedOutputStream* outputStream) {});

    field.append().setFloat(1);

    testEncode([&](int fieldKey, google::protobuf::io::CodedOutputStream* outputStream) {
        google::protobuf::RepeatedField<float> values;
        values.Add(1.0f);

        writeRepeatedField(fieldKey, outputStream, values, &writeFloat);
    });

    field.append().setFloat(2.0f);
    field.append().setFloat(345678901.0f);

    testEncode([&](int fieldKey, google::protobuf::io::CodedOutputStream* outputStream) {
        google::protobuf::RepeatedField<float> values;
        values.Add(1.0f);
        values.Add(2.0f);
        values.Add(345678901.0f);

        writeRepeatedField(fieldKey, outputStream, values, &writeFloat);
    });
}

TEST_F(FieldTest, supportsPackedFixed64Repeated) {
    testEncode([&](int fieldKey, google::protobuf::io::CodedOutputStream* outputStream) {});

    field.append().setDouble(1);

    testEncode([&](int fieldKey, google::protobuf::io::CodedOutputStream* outputStream) {
        google::protobuf::RepeatedField<double> values;
        values.Add(1.0f);

        writeRepeatedField(fieldKey, outputStream, values, &writeDouble);
    });

    field.append().setDouble(2.0f);
    field.append().setDouble(345678901.0f);

    testEncode([&](int fieldKey, google::protobuf::io::CodedOutputStream* outputStream) {
        google::protobuf::RepeatedField<double> values;
        values.Add(1.0f);
        values.Add(2.0f);
        values.Add(345678901.0f);

        writeRepeatedField(fieldKey, outputStream, values, &writeDouble);
    });
}

TEST_F(FieldTest, supportsMessage) {
    auto message = makeShared<Protobuf::Message>();
    message->getOrCreateField(11).setInt32(0);
    message->getOrCreateField(10).setDouble(4.0);

    field.setMessage(message.get());

    ASSERT_EQ(2, message.use_count());

    ASSERT_EQ(message.get(), field.getMessage());

    testEncode([&](int fieldKey, google::protobuf::io::CodedOutputStream* outputStream) {
        std::string target;
        google::protobuf::io::StringOutputStream stream(&target);
        google::protobuf::io::CodedOutputStream messageOutputStream(&stream, false);

        WireFormat::WriteDouble(10, 4.0, &messageOutputStream);

        auto writtenCount = static_cast<size_t>(messageOutputStream.ByteCount());

        WireFormat::WriteBytes(fieldKey, std::string(target.data(), writtenCount), outputStream);
    });
}

} // namespace
