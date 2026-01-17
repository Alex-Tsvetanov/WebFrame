#include "protogen/test.pb.h"
#include "valdi_protobuf/Message.hpp"
#include "gtest/gtest.h"

using namespace Valdi;
namespace {

/**
 message OtherMessage {
   string value = 1;
 }

 message Message {
   int32 int32 = 1;
   int64 int64 = 2;
   uint32 uint32 = 3;
   uint64 uint64 = 4;
   sint32 sint32 = 5;
   sint64 sint64 = 6;
   fixed32 fixed32 = 7;
   fixed64 fixed64 = 8;
   sfixed32 sfixed32 = 9;
   sfixed64 sfixed64 = 10;
   float float = 11;
   double double = 12;
   bool bool = 13;
   string string = 14;
   bytes bytes = 15;
   Enum enum = 16;
   Message self_message = 17;
   OtherMessage other_message = 18;
 }
 */

TEST(Message, canDecodeSingle) {
    test::Message message;

    message.set_int32(42);
    message.set_int64(1337133713371337);
    message.set_uint32(4294967294);
    message.set_uint64(104294967296u);
    message.set_sint32(-43);
    message.set_sint64(-1337133713371337);
    message.set_fixed32(42);
    message.set_fixed64(10429496729600u);
    message.set_sfixed32(-42);
    message.set_sfixed64(-10429496729600);
    message.set_float_(0.98765);
    message.set_double_(0.987654321);
    message.set_bool_(true);
    message.set_string("Hello World and Welcome!");
    message.set_bytes("<some bytes here>");
    message.set_enum_(test::Enum::VALUE_1);

    auto bytes = message.ByteSizeLong();
    auto buffer = makeShared<ByteBuffer>();
    buffer->resize(bytes);

    ASSERT_TRUE(message.SerializeToArray(buffer->data(), buffer->size()));

    SimpleExceptionTracker exceptionTracker;
    auto parsedMessage = Protobuf::Message::parse(buffer->toBytesView(), message.GetDescriptor(), exceptionTracker);

    ASSERT_EQ(std::vector<Protobuf::FieldNumber>({1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16}),
              parsedMessage->sortedFieldNumbers());

    ASSERT_EQ(42, parsedMessage->getOrCreateField(1).getInt32());
    ASSERT_EQ(1337133713371337, parsedMessage->getOrCreateField(2).getInt64());
    ASSERT_EQ(4294967294u, parsedMessage->getOrCreateField(3).getUInt32());
    ASSERT_EQ(104294967296u, parsedMessage->getOrCreateField(4).getUInt64());
    ASSERT_EQ(-43, parsedMessage->getOrCreateField(5).getSInt32());
    ASSERT_EQ(-1337133713371337, parsedMessage->getOrCreateField(6).getSInt64());
    ASSERT_EQ(42u, parsedMessage->getOrCreateField(7).getUInt32());
    ASSERT_EQ(10429496729600u, parsedMessage->getOrCreateField(8).getUInt64());
    ASSERT_EQ(-42, parsedMessage->getOrCreateField(9).getInt32());
    ASSERT_EQ(-10429496729600, parsedMessage->getOrCreateField(10).getInt64());
    ASSERT_EQ(0.98765f, parsedMessage->getOrCreateField(11).getFloat());
    ASSERT_EQ(0.987654321, parsedMessage->getOrCreateField(12).getDouble());
    ASSERT_EQ(true, parsedMessage->getOrCreateField(13).getBool());

    ASSERT_EQ("Hello World and Welcome!", parsedMessage->getOrCreateField(14).getRaw().toStringView());
    ASSERT_EQ("<some bytes here>", parsedMessage->getOrCreateField(15).getRaw().toStringView());
}

TEST(Message, canEncodeSingle) {
    auto message = makeShared<Protobuf::Message>();

    message->getOrCreateField(1).setInt32(42);
    message->getOrCreateField(2).setInt64(1337133713371337);
    message->getOrCreateField(3).setUInt32(4294967294u);
    message->getOrCreateField(4).setUInt64(104294967296u);
    message->getOrCreateField(5).setSInt32(-43);
    message->getOrCreateField(6).setSInt64(-1337133713371337);
    message->getOrCreateField(7) = Protobuf::Field::fixed32(42u);
    message->getOrCreateField(8) = Protobuf::Field::fixed64(10429496729600u);
    message->getOrCreateField(9) = Protobuf::Field::fixed32(static_cast<uint32_t>(-42));
    message->getOrCreateField(10) = Protobuf::Field::fixed64(static_cast<uint64_t>(-10429496729600));
    message->getOrCreateField(11).setFloat(0.98765f);
    message->getOrCreateField(12).setDouble(0.987654321);
    message->getOrCreateField(13).setBool(true);
    auto str = StaticString::makeUTF8("Hello World and Welcome!");
    message->getOrCreateField(14).setString(str.get());

    auto buffer = makeShared<ByteBuffer>();
    buffer->append("<some bytes here>");
    auto typedArray = makeShared<ValueTypedArray>(kDefaultTypedArrayType, buffer->toBytesView());
    message->getOrCreateField(15).setTypedArray(typedArray.get());

    auto result = message->encode();

    test::Message parsedMessage;
    ASSERT_TRUE(parsedMessage.ParseFromArray(result.data(), result.size()));

    ASSERT_EQ(42, parsedMessage.int32());
    ASSERT_EQ(1337133713371337, parsedMessage.int64());
    ASSERT_EQ(4294967294u, parsedMessage.uint32());
    ASSERT_EQ(104294967296u, parsedMessage.uint64());
    ASSERT_EQ(-43, parsedMessage.sint32());
    ASSERT_EQ(-1337133713371337, parsedMessage.sint64());
    ASSERT_EQ(42u, parsedMessage.fixed32());
    ASSERT_EQ(10429496729600u, parsedMessage.fixed64());
    ASSERT_EQ(-42, parsedMessage.sfixed32());
    ASSERT_EQ(-10429496729600, parsedMessage.sfixed64());
    ASSERT_EQ(0.98765f, parsedMessage.float_());
    ASSERT_EQ(0.987654321, parsedMessage.double_());
    ASSERT_EQ(true, parsedMessage.bool_());

    // Check that the encoding is the same as the official Google's protobuf impl
    auto parsedMessageEncodeed = parsedMessage.SerializeAsString();
    ASSERT_EQ(result.asStringView(), std::string_view(parsedMessageEncodeed));
}

TEST(Message, canDecodeNested) {
    test::Message message;

    message.set_int32(-42);
    message.set_int64(-1337133713371337);
    message.set_sint32(43);
    message.set_sint64(1337133713371337);
    message.set_sfixed32(42);
    message.set_sfixed64(10429496729600);

    auto* otherMessage = message.mutable_other_message();
    otherMessage->set_value("Hello World!");

    auto bytes = message.ByteSizeLong();
    auto buffer = makeShared<ByteBuffer>();
    buffer->resize(bytes);

    ASSERT_TRUE(message.SerializeToArray(buffer->data(), buffer->size()));

    SimpleExceptionTracker exceptionTracker;
    auto parsedMessage = Protobuf::Message::parse(buffer->toBytesView(), message.GetDescriptor(), exceptionTracker);

    ASSERT_TRUE(parsedMessage != nullptr);

    ASSERT_EQ(std::vector<Protobuf::FieldNumber>({1, 2, 5, 6, 9, 10, 18}), parsedMessage->sortedFieldNumbers());

    ASSERT_EQ(-42, parsedMessage->getOrCreateField(1).getInt32());
    ASSERT_EQ(-1337133713371337, parsedMessage->getOrCreateField(2).getInt64());
    ASSERT_EQ(43, parsedMessage->getOrCreateField(5).getSInt32());
    ASSERT_EQ(1337133713371337, parsedMessage->getOrCreateField(6).getSInt64());
    ASSERT_EQ(42, parsedMessage->getOrCreateField(9).getInt32());
    ASSERT_EQ(10429496729600, parsedMessage->getOrCreateField(10).getInt64());

    auto* parsedOtherMessage = parsedMessage->getOrCreateField(18).getMessage();

    ASSERT_TRUE(parsedOtherMessage == nullptr);

    ASSERT_TRUE(parsedMessage->postprocess(true, exceptionTracker));

    parsedOtherMessage = parsedMessage->getOrCreateField(18).getMessage();

    ASSERT_TRUE(parsedOtherMessage != nullptr);

    ASSERT_EQ("Hello World!", parsedOtherMessage->getOrCreateField(1).getRaw().toStringView());
}

TEST(Message, canEncodeNested) {
    auto message = makeShared<Protobuf::Message>();
    message->getOrCreateField(1).setInt32(42);

    auto submessage = makeShared<Protobuf::Message>();

    message->getOrCreateField(1).setInt32(42);
    message->getOrCreateField(2).setInt64(1337133713371337);
    message->getOrCreateField(3).setUInt32(4294967294u);
    message->getOrCreateField(4).setUInt64(104294967296u);
    message->getOrCreateField(5).setSInt32(-43);
    message->getOrCreateField(6).setSInt64(-1337133713371337);
    message->getOrCreateField(7) = Protobuf::Field::fixed32(42u);
    message->getOrCreateField(8) = Protobuf::Field::fixed64(10429496729600u);
    message->getOrCreateField(9) = Protobuf::Field::fixed32(static_cast<uint32_t>(-42));
    message->getOrCreateField(10) = Protobuf::Field::fixed64(static_cast<uint64_t>(-10429496729600));
    message->getOrCreateField(11).setFloat(0.98765f);
    message->getOrCreateField(12).setDouble(0.987654321);
    message->getOrCreateField(13).setBool(true);
    message->getOrCreateField(18).setMessage(submessage.get());
    auto str = StaticString::makeUTF8("Hello World");
    submessage->getOrCreateField(1).setString(str.get());

    auto result = message->encode();

    test::Message parsedMessage;
    ASSERT_TRUE(parsedMessage.ParseFromArray(result.data(), result.size()));

    ASSERT_EQ(42, parsedMessage.int32());
    ASSERT_EQ(1337133713371337, parsedMessage.int64());
    ASSERT_EQ(4294967294u, parsedMessage.uint32());
    ASSERT_EQ(104294967296u, parsedMessage.uint64());
    ASSERT_EQ(-43, parsedMessage.sint32());
    ASSERT_EQ(-1337133713371337, parsedMessage.sint64());
    ASSERT_EQ(42u, parsedMessage.fixed32());
    ASSERT_EQ(10429496729600u, parsedMessage.fixed64());
    ASSERT_EQ(-42, parsedMessage.sfixed32());
    ASSERT_EQ(-10429496729600, parsedMessage.sfixed64());
    ASSERT_EQ(0.98765f, parsedMessage.float_());
    ASSERT_EQ(0.987654321, parsedMessage.double_());
    ASSERT_EQ(true, parsedMessage.bool_());

    ASSERT_TRUE(parsedMessage.has_other_message());
    const auto& otherMessage = parsedMessage.other_message();
    ASSERT_EQ("Hello World", otherMessage.value());

    // Check that the encoding is the same as the official Google's protobuf impl
    auto parsedMessageEncodeed = parsedMessage.SerializeAsString();
    ASSERT_EQ(result.asStringView(), std::string_view(parsedMessageEncodeed));
}

TEST(Message, encodesEmptyNestedMessage) {
    auto message = makeShared<Protobuf::Message>();
    auto submessage = makeShared<Protobuf::Message>();
    message->getOrCreateField(18).setMessage(submessage.get());

    auto result = message->encode();

    test::Message emptyMessage;
    emptyMessage.mutable_other_message();
    auto emptyMessageEncoded = emptyMessage.SerializeAsString();

    ASSERT_EQ(result.asStringView(), std::string_view(emptyMessageEncoded));
}

TEST(Message, canEncodeRepeated) {
    auto message = makeShared<Protobuf::Message>();

    Protobuf::Field field;
    field.setInt32(10);
    message->appendField(1, field);
    field.setInt32(20);
    message->appendField(1, field);
    field.setInt32(30);
    message->appendField(1, field);

    auto result = message->encode();

    test::RepeatedMessage parsedMessage;
    ASSERT_TRUE(parsedMessage.ParseFromArray(result.data(), result.size()));

    ASSERT_EQ(3, parsedMessage.int32_size());
    ASSERT_EQ(10, parsedMessage.int32(0));
    ASSERT_EQ(20, parsedMessage.int32(1));
    ASSERT_EQ(30, parsedMessage.int32(2));

    // Check that the encoding is the same as the official Google's protobuf impl
    auto parsedMessageEncodeed = parsedMessage.SerializeAsString();
    ASSERT_EQ(result.asStringView(), std::string_view(parsedMessageEncodeed));
}

TEST(Message, canDecodeRepeated) {
    test::RepeatedMessage message;
    message.add_int32(10);
    message.add_int32(20);
    message.add_int32(30);

    auto bytes = message.ByteSizeLong();
    auto buffer = makeShared<ByteBuffer>();
    buffer->resize(bytes);

    ASSERT_TRUE(message.SerializeToArray(buffer->data(), buffer->size()));

    SimpleExceptionTracker exceptionTracker;
    auto parsedMessage = Protobuf::Message::parse(buffer->toBytesView(), message.GetDescriptor(), exceptionTracker);

    ASSERT_TRUE(parsedMessage != nullptr);

    ASSERT_EQ(std::vector<Protobuf::FieldNumber>({1}), parsedMessage->sortedFieldNumbers());

    auto* repeated = parsedMessage->getOrCreateField(1).toVarintRepeated();

    ASSERT_EQ(static_cast<size_t>(3), repeated->size());
    ASSERT_EQ(10, (*repeated)[0].getInt32());
    ASSERT_EQ(20, (*repeated)[1].getInt32());
    ASSERT_EQ(30, (*repeated)[2].getInt32());
}

TEST(Message, serializesOneOfFieldEvenWithDefaultValue) {
    test::OneOfMessage message;

    message.set_string_1("");

    auto bytes = message.ByteSizeLong();
    auto buffer = makeShared<ByteBuffer>();
    buffer->resize(bytes);

    ASSERT_TRUE(message.SerializeToArray(buffer->data(), buffer->size()));

    SimpleExceptionTracker exceptionTracker;
    auto parsedMessage = Protobuf::Message::parse(buffer->toBytesView(), message.GetDescriptor(), exceptionTracker);

    ASSERT_TRUE(parsedMessage != nullptr);

    ASSERT_TRUE(parsedMessage->postprocess(true, exceptionTracker));

    ASSERT_EQ(std::vector<Protobuf::FieldNumber>({2}), parsedMessage->sortedFieldNumbers());

    auto& field = parsedMessage->getOrCreateField(2);

    ASSERT_EQ(Protobuf::Field::InternalType::Raw, field.getInternalType());
    ASSERT_EQ("", field.getRaw().toStringView());

    auto result = parsedMessage->encode();

    test::OneOfMessage parsedPbMessage;
    ASSERT_TRUE(parsedPbMessage.ParseFromArray(result.data(), result.size()));

    ASSERT_EQ(test::OneOfMessage::StringsCase::kString1, parsedPbMessage.strings_case());
}

TEST(Message, ignoresEmptyFields) {
    auto message = makeShared<Protobuf::Message>();
    auto nestedMessage = makeShared<Protobuf::Message>();

    message->getOrCreateField(1).setInt32(0);
    message->getOrCreateField(2).setMessage(nestedMessage.get());

    nestedMessage->getOrCreateField(4).setBool(false);

    auto output = message->encode(false);

    auto decoded = Protobuf::Message::parse(output, nullptr);
    ASSERT_TRUE(decoded) << decoded.description();

    auto* field = decoded.value()->getField(1);
    ASSERT_TRUE(field == nullptr);

    auto* messageField = decoded.value()->getField(2);
    ASSERT_TRUE(messageField != nullptr);

    auto decodedNestedMessage = Protobuf::Message::parse(
        BytesView(nullptr, messageField->getRaw().data, messageField->getRaw().length), nullptr);
    ASSERT_TRUE(decodedNestedMessage) << decodedNestedMessage.description();

    auto* nestedField = decodedNestedMessage.value()->getField(4);
    ASSERT_TRUE(nestedField == nullptr);
}

TEST(Message, canEncodeWhileKeepingEmptyFields) {
    auto message = makeShared<Protobuf::Message>();
    auto nestedMessage = makeShared<Protobuf::Message>();

    message->getOrCreateField(1).setInt32(0);
    message->getOrCreateField(2).setMessage(nestedMessage.get());

    nestedMessage->getOrCreateField(4).setBool(false);

    auto output = message->encode(true);

    auto decoded = Protobuf::Message::parse(output, nullptr);
    ASSERT_TRUE(decoded) << decoded.description();

    auto* field = decoded.value()->getField(1);
    ASSERT_TRUE(field != nullptr);
    ASSERT_EQ(static_cast<int32_t>(0), field->getInt32());

    auto* messageField = decoded.value()->getField(2);
    ASSERT_TRUE(messageField != nullptr);

    auto decodedNestedMessage = Protobuf::Message::parse(
        BytesView(nullptr, messageField->getRaw().data, messageField->getRaw().length), nullptr);
    ASSERT_TRUE(decodedNestedMessage) << decodedNestedMessage.description();

    auto* nestedField = decodedNestedMessage.value()->getField(4);
    ASSERT_TRUE(nestedField != nullptr);

    ASSERT_EQ(false, nestedField->getBool());
}

TEST(Message, canConvertToJSON) {
    test::Message messagePrototype;
    auto message = makeShared<Protobuf::Message>(messagePrototype.GetDescriptor(), nullptr);

    message->getOrCreateField(1).setInt32(42);
    message->getOrCreateField(2).setInt64(1337133713371337);
    message->getOrCreateField(7) = Protobuf::Field::fixed32(42u);
    message->getOrCreateField(8) = Protobuf::Field::fixed64(10429496729600u);
    message->getOrCreateField(12).setDouble(0.987654321);
    message->getOrCreateField(13).setBool(true);

    SimpleExceptionTracker exceptionTracker;
    auto json = message->toJSON(Protobuf::JSONPrintOptions(), exceptionTracker);

    ASSERT_TRUE(exceptionTracker);

    ASSERT_EQ("{\"int32\":42,\"int64\":\"1337133713371337\",\"fixed32\":42,\"fixed64\":\"10429496729600\",\"double\":0."
              "987654321,\"bool\":true}",
              json);
}

TEST(Message, canParseFromJSON) {
    test::Message messagePrototype;

    auto result =
        Protobuf::Message::parseFromJSON("{\"int32\":42,\"int64\":\"1337133713371337\",\"fixed32\":42,\"fixed64\":"
                                         "\"10429496729600\",\"double\":0.987654321,\"bool\":true}",
                                         messagePrototype.GetDescriptor());
    ASSERT_TRUE(result) << result.description();

    auto message = result.moveValue();

    ASSERT_EQ(42, message->getOrCreateField(1).getInt32());
    ASSERT_EQ(1337133713371337, message->getOrCreateField(2).getInt64());
    ASSERT_EQ(42u, message->getOrCreateField(7).getFixed32());
    ASSERT_EQ(10429496729600u, message->getOrCreateField(8).getFixed64());
    ASSERT_EQ(0.987654321, message->getOrCreateField(12).getDouble());
    ASSERT_EQ(true, message->getOrCreateField(13).getBool());
}

} // namespace
