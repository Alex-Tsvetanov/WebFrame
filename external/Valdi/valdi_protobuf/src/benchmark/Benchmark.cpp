#include "protogen/test.pb.h"
#include "valdi_core/cpp/Utils/ByteBuffer.hpp"
#include "valdi_protobuf/Message.hpp"
#include <benchmark/benchmark.h>
#include <google/protobuf/dynamic_message.h>

using namespace Valdi;

static BytesView makeProtoData() {
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

    std::string messageBytes;
    while (messageBytes.length() < 2048) {
        messageBytes += "abcdefghijklmnopqrstuvwxyz";
    }
    message.set_bytes(std::move(messageBytes));
    message.set_enum_(test::Enum::VALUE_1);

    auto* otherMessage = message.mutable_other_message();
    otherMessage->set_value("Hello World!");

    auto bytes = message.ByteSizeLong();
    auto buffer = makeShared<ByteBuffer>();
    buffer->resize(bytes);

    SC_ASSERT(message.SerializeToArray(buffer->data(), buffer->size()));

    return buffer->toBytesView();
}

static void DecodeProtobufCpp(benchmark::State& state) {
    auto protoData = makeProtoData();

    for (auto _ : state) {
        auto message = makeShared<test::Message>();
        if (!message->ParseFromArray(protoData.data(), protoData.size())) {
            SC_ABORT("Message failed to parse");
        }
    }
}
BENCHMARK(DecodeProtobufCpp);

static void DecodeProtobufReflectionCpp(benchmark::State& state) {
    auto protoData = makeProtoData();
    google::protobuf::DynamicMessageFactory factory;
    auto* prototype = factory.GetPrototype(test::Message::GetDescriptor());

    for (auto _ : state) {
        auto message = prototype->New();
        if (!message->ParseFromArray(protoData.data(), protoData.size())) {
            SC_ABORT("Message failed to parse");
        }
    }
}
BENCHMARK(DecodeProtobufReflectionCpp);

static void EncodeProtobufCpp(benchmark::State& state) {
    auto protoData = makeProtoData();
    auto message = makeShared<test::Message>();
    if (!message->ParseFromArray(protoData.data(), protoData.size())) {
        SC_ABORT("Message failed to parse");
    }

    for (auto _ : state) {
        auto buffer = makeShared<ByteBuffer>();
        buffer->resize(message->ByteSizeLong());

        if (!message->SerializeToArray(buffer->data(), buffer->size())) {
            SC_ABORT("Message failed to serialize");
        }
    }
}
BENCHMARK(EncodeProtobufCpp);

static void EncodeProtobufReflectionCpp(benchmark::State& state) {
    auto protoData = makeProtoData();
    google::protobuf::DynamicMessageFactory factory;
    auto* prototype = factory.GetPrototype(test::Message::GetDescriptor());
    auto message = prototype->New();
    if (!message->ParseFromArray(protoData.data(), protoData.size())) {
        SC_ABORT("Message failed to parse");
    }

    for (auto _ : state) {
        auto buffer = makeShared<ByteBuffer>();
        buffer->resize(message->ByteSizeLong());

        if (!message->SerializeToArray(buffer->data(), buffer->size())) {
            SC_ABORT("Message failed to serialize");
        }
    }
}
BENCHMARK(EncodeProtobufReflectionCpp);

static void DecodeValdiProtobuf(benchmark::State& state) {
    auto protoData = makeProtoData();
    const auto& descriptor = *test::Message::GetDescriptor();

    for (auto _ : state) {
        auto message = Protobuf::Message::parse(protoData, descriptor);
        if (!message) {
            SC_ABORT("Message failed to parse");
        }
    }
}
BENCHMARK(DecodeValdiProtobuf);

static void DecodeValdiProtobufNoPostprocess(benchmark::State& state) {
    auto protoData = makeProtoData();

    for (auto _ : state) {
        auto message = Protobuf::Message::parse(protoData);
        if (!message) {
            SC_ABORT("Message failed to parse");
        }
    }
}
BENCHMARK(DecodeValdiProtobufNoPostprocess);

static void EncodeValdiProtobuf(benchmark::State& state) {
    auto protoData = makeProtoData();
    const auto& descriptor = *test::Message::GetDescriptor();
    auto result = Protobuf::Message::parse(protoData, descriptor);
    if (!result) {
        SC_ABORT("Message failed to parse");
    }
    auto message = result.moveValue();

    for (auto _ : state) {
        auto result = message->encode();
        if (!result) {
            SC_ABORT("Message failed to serialize");
        }
    }
}
BENCHMARK(EncodeValdiProtobuf);

BENCHMARK_MAIN();
