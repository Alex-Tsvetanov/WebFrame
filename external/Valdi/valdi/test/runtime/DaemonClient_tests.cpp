//
//  DaemonClient_tests.cpp
//  valdi-pc
//
//  Created by Simon Corsin on 10/8/19.
//

#include "valdi/runtime/Debugger/DaemonClient.hpp"
#include "valdi/runtime/Resources/Resource.hpp"
#include "valdi/valdi.pb.h"
#include "valdi_core/cpp/Resources/ValdiPacket.hpp"
#include "valdi_core/cpp/Utils/ConsoleLogger.hpp"
#include "valdi_core/cpp/Utils/StringCache.hpp"
#include "valdi_core/cpp/Utils/ValueFunctionWithCallable.hpp"
#include "valdi_core/cpp/Utils/ValueUtils.hpp"
#include <deque>
#include <gtest/gtest.h>

#include "utils/encoding/Base64Utils.hpp"

using namespace Valdi;

namespace ValdiTest {

class MockDaemonClientListener : public DaemonClientListener {
public:
    ~MockDaemonClientListener() override = default;

    void didReceiveUpdatedResources(const SharedVector<Shared<Resource>>& resources) override {
        didReceiveUpdatedResourcesParams.emplace_back(resources);
    }

    void daemonClientDidDisconnect(DaemonClient* daemonClient, const Error& error) override {
        daemonClientDidDisconnectParams.emplace_back(error);
    }

    void didReceiveClientPayload(DaemonClient* daemonClient, int senderClientId, const BytesView& payload) override {}

    std::vector<SharedVector<Shared<Resource>>> didReceiveUpdatedResourcesParams;
    std::vector<Error> daemonClientDidDisconnectParams;
};

class MockDaemonClientDataSender : public DaemonClientDataSender {
public:
    ~MockDaemonClientDataSender() override = default;

    void submitData(const BytesView& data) override {
        submittedData.emplace_back(data);
    }

    bool requiresPacketEncoding() const override {
        return true;
    }

    Result<Value> dequeueSubmittedPayload() {
        if (submittedData.empty()) {
            return Error("No submitted data");
        }

        auto bytes = submittedData.front();
        submittedData.pop_front();

        auto packet = ValdiPacket::read(bytes.data(), bytes.size());
        if (!packet) {
            return packet.moveError();
        }

        return jsonToValue(packet.value().data(), static_cast<int>(packet.value().size()));
    }

private:
    std::deque<BytesView> submittedData;
};

TEST(DaemonClient, canReceiveUpdatedResources) {
    MockDaemonClientListener listener;
    auto daemonClient =
        Valdi::makeShared<DaemonClient>(&listener, 0, Valdi::strongSmallRef(&ConsoleLogger::getLogger()));

    auto updatedResources = ValueArray::make(2);
    Value resource1;
    auto base64Data1 = Value(snap::utils::encoding::binaryToBase64("hello"));
    resource1.setMapValue("data_base64", base64Data1)
        .setMapValue("bundle_name", Value("bundle1"))
        .setMapValue("file_path_within_bundle", Value("src/file1.js"));
    updatedResources->emplace(0, resource1);

    Value resource2;
    auto base64Data2 = Value(snap::utils::encoding::binaryToBase64("hello2"));
    resource2.setMapValue("data_base64", base64Data2)
        .setMapValue("bundle_name", Value("bundle2"))
        .setMapValue("file_path_within_bundle", Value("src/file2.js"));
    updatedResources->emplace(1, resource2);

    Value resources;
    resources.setMapValue("resources", Value(updatedResources));
    Value event;
    event.setMapValue("updated_resources", resources);
    Value payload;
    payload.setMapValue("event", event);
    auto output = valueToJson(payload);

    auto out = makeShared<ByteBuffer>();

    ValdiPacket::write(reinterpret_cast<const Byte*>(output->data()), output->size(), *out);

    daemonClient->onDataReceived(out->toBytesView());

    ASSERT_TRUE(listener.daemonClientDidDisconnectParams.empty());
    ASSERT_EQ(static_cast<size_t>(1), listener.didReceiveUpdatedResourcesParams.size());

    auto receivedResources = listener.didReceiveUpdatedResourcesParams[0];
    ASSERT_TRUE(receivedResources != nullptr);

    ASSERT_EQ(static_cast<size_t>(2), receivedResources->size());

    ASSERT_EQ(ResourceId(STRING_LITERAL("bundle1"), STRING_LITERAL("src/file1.js")),
              (*receivedResources)[0]->resourceId);
    ASSERT_EQ(STRING_LITERAL("hello").toStringView(), BytesView((*receivedResources)[0]->data).asStringView());
    ASSERT_EQ(ResourceId(STRING_LITERAL("bundle2"), STRING_LITERAL("src/file2.js")),
              (*receivedResources)[1]->resourceId);
    ASSERT_EQ(STRING_LITERAL("hello2").toStringView(), BytesView((*receivedResources)[1]->data).asStringView());
}

TEST(DaemonClient, canHandleGroupedPackets) {
    MockDaemonClientListener listener;
    auto daemonClient =
        Valdi::makeShared<DaemonClient>(&listener, 0, Valdi::strongSmallRef(&ConsoleLogger::getLogger()));

    auto updatedResources1 = ValueArray::make(1);
    Value resource1;
    auto base64Data1 = Value(snap::utils::encoding::binaryToBase64("hello"));
    resource1.setMapValue("data_base64", base64Data1)
        .setMapValue("bundle_name", Value("bundle1"))
        .setMapValue("file_path_within_bundle", Value("src/file1.js"));
    updatedResources1->emplace(0, resource1);
    Value resources1;
    resources1.setMapValue("resources", Value(updatedResources1));
    Value event1;
    event1.setMapValue("updated_resources", resources1);
    Value payload1;
    payload1.setMapValue("event", event1);

    auto updatedResources2 = ValueArray::make(1);
    Value resource2;
    auto base64Data2 = Value(snap::utils::encoding::binaryToBase64("hello2"));
    resource2.setMapValue("data_base64", base64Data2)
        .setMapValue("bundle_name", Value("bundle2"))
        .setMapValue("file_path_within_bundle", Value("src/file2.js"));
    updatedResources2->emplace(0, resource2);
    Value resources2;
    resources2.setMapValue("resources", Value(updatedResources2));
    Value event2;
    event2.setMapValue("updated_resources", resources2);
    Value payload2;
    payload2.setMapValue("event", event2);

    auto output1 = valueToJson(payload1);
    auto output2 = valueToJson(payload2);

    auto out = makeShared<ByteBuffer>();
    ValdiPacket::write(reinterpret_cast<const Byte*>(output1->data()), output1->size(), *out);
    ValdiPacket::write(reinterpret_cast<const Byte*>(output2->data()), output2->size(), *out);

    daemonClient->onDataReceived(out->toBytesView());

    ASSERT_TRUE(listener.daemonClientDidDisconnectParams.empty());
    ASSERT_EQ(static_cast<size_t>(2), listener.didReceiveUpdatedResourcesParams.size());

    auto receivedResources1 = listener.didReceiveUpdatedResourcesParams[0];
    ASSERT_TRUE(receivedResources1 != nullptr);

    ASSERT_EQ(static_cast<size_t>(1), receivedResources1->size());

    ASSERT_EQ(ResourceId(STRING_LITERAL("bundle1"), STRING_LITERAL("src/file1.js")),
              (*receivedResources1)[0]->resourceId);
    ASSERT_EQ(STRING_LITERAL("hello").toStringView(), BytesView((*receivedResources1)[0]->data).asStringView());

    auto receivedResources2 = listener.didReceiveUpdatedResourcesParams[1];
    ASSERT_TRUE(receivedResources2 != nullptr);

    ASSERT_EQ(static_cast<size_t>(1), receivedResources2->size());

    ASSERT_EQ(ResourceId(STRING_LITERAL("bundle2"), STRING_LITERAL("src/file2.js")),
              (*receivedResources2)[0]->resourceId);
    ASSERT_EQ(STRING_LITERAL("hello2").toStringView(), BytesView((*receivedResources2)[0]->data).asStringView());
}

TEST(DaemonClient, canHandleIncompletePackets) {
    MockDaemonClientListener listener;
    auto daemonClient =
        Valdi::makeShared<DaemonClient>(&listener, 0, Valdi::strongSmallRef(&ConsoleLogger::getLogger()));

    auto updatedResources = ValueArray::make(1);
    Value resource1;
    auto base64Data1 = Value(snap::utils::encoding::binaryToBase64("hello"));
    resource1.setMapValue("data_base64", base64Data1)
        .setMapValue("bundle_name", Value("bundle1"))
        .setMapValue("file_path_within_bundle", Value("src/file1.js"));
    updatedResources->emplace(0, resource1);

    Value resources;
    resources.setMapValue("resources", Value(updatedResources));
    Value event;
    event.setMapValue("updated_resources", resources);
    Value payload;
    payload.setMapValue("event", event);
    auto output = valueToJson(payload);

    auto out = makeShared<ByteBuffer>();

    ValdiPacket::write(reinterpret_cast<const Byte*>(output->data()), output->size(), *out);

    auto packet1 = makeShared<ByteBuffer>(out->begin(), out->begin() + 2);
    auto packet2 = makeShared<ByteBuffer>(out->begin() + 2, out->begin() + ValdiPacket::minSize() + 2);
    auto packet3 = makeShared<ByteBuffer>(out->begin() + ValdiPacket::minSize() + 2, out->end());

    daemonClient->onDataReceived(packet1->toBytesView());
    ASSERT_TRUE(listener.didReceiveUpdatedResourcesParams.empty());

    daemonClient->onDataReceived(packet2->toBytesView());
    ASSERT_TRUE(listener.didReceiveUpdatedResourcesParams.empty());

    daemonClient->onDataReceived(packet3->toBytesView());
    ASSERT_FALSE(listener.didReceiveUpdatedResourcesParams.empty());

    ASSERT_TRUE(listener.daemonClientDidDisconnectParams.empty());
    ASSERT_EQ(static_cast<size_t>(1), listener.didReceiveUpdatedResourcesParams.size());

    auto updatedResources1 = listener.didReceiveUpdatedResourcesParams[0];
    ASSERT_TRUE(updatedResources1 != nullptr);

    ASSERT_EQ(static_cast<size_t>(1), updatedResources1->size());

    ASSERT_EQ(ResourceId(STRING_LITERAL("bundle1"), STRING_LITERAL("src/file1.js")),
              (*updatedResources1)[0]->resourceId);
    ASSERT_EQ(STRING_LITERAL("hello").toStringView(), BytesView((*updatedResources1)[0]->data).asStringView());
}

TEST(DaemonClient, disconnectOnInvalidPacket) {
    MockDaemonClientListener listener;
    auto daemonClient =
        Valdi::makeShared<DaemonClient>(&listener, 0, Valdi::strongSmallRef(&ConsoleLogger::getLogger()));

    auto out = makeShared<ByteBuffer>();

    for (size_t i = 0; i < 32; i++) {
        out->append(static_cast<Byte>(42));
    }

    daemonClient->onDataReceived(out->toBytesView());
    ASSERT_TRUE(listener.didReceiveUpdatedResourcesParams.empty());
    ASSERT_FALSE(listener.daemonClientDidDisconnectParams.empty());
    ASSERT_EQ(static_cast<size_t>(1), listener.daemonClientDidDisconnectParams.size());
}

TEST(DaemonClient, canSendConfiguration) {
    MockDaemonClientListener listener;
    auto dataSender = makeShared<MockDaemonClientDataSender>();
    auto daemonClient =
        Valdi::makeShared<DaemonClient>(&listener, 0, Valdi::strongSmallRef(&ConsoleLogger::getLogger()));
    daemonClient->setDataSender(dataSender);

    daemonClient->sendConfiguration(STRING_LITERAL("MyApplication"), snap::valdi_core::Platform::Android, true);

    auto result = dataSender->dequeueSubmittedPayload();
    ASSERT_TRUE(result.success()) << result.description();

    auto& sentPayload = result.value();

    ASSERT_FALSE(sentPayload.getMapValue("request").isNullOrUndefined());
    auto request = sentPayload.getMapValue("request");
    ASSERT_EQ(STRING_LITERAL("1"), request.getMapValue("request_id").toStringBox());
    ASSERT_FALSE(request.getMapValue("configure").isNullOrUndefined());
    auto configure = request.getMapValue("configure");

    ASSERT_EQ(STRING_LITERAL("MyApplication"), configure.getMapValue("application_id").toStringBox());
    ASSERT_EQ(STRING_LITERAL("android"), configure.getMapValue("platform").toStringBox());
    ASSERT_EQ(Value(true), configure.getMapValue("disable_hot_reload"));
}

TEST(DaemonClient, canSendLog) {
    MockDaemonClientListener listener;
    auto dataSender = makeShared<MockDaemonClientDataSender>();
    auto daemonClient =
        Valdi::makeShared<DaemonClient>(&listener, 0, Valdi::strongSmallRef(&ConsoleLogger::getLogger()));
    daemonClient->setDataSender(dataSender);

    daemonClient->sendLog(LogTypeDebug, "This is a log");

    auto result = dataSender->dequeueSubmittedPayload();
    ASSERT_TRUE(result.success()) << result.description();

    auto& sentPayload = result.value();

    ASSERT_FALSE(sentPayload.getMapValue("event").isNullOrUndefined());
    auto event = sentPayload.getMapValue("event");
    ASSERT_FALSE(event.getMapValue("new_logs").isNullOrUndefined());
    auto logs = event.getMapValue("new_logs").getArray();
    ASSERT_EQ(static_cast<size_t>(1), logs->size());

    auto log = (*logs)[0];

    ASSERT_EQ(STRING_LITERAL("debug"), log.getMapValue("severity").toStringBox());
    ASSERT_EQ(STRING_LITERAL("This is a log"), log.getMapValue("log").toStringBox());
}

} // namespace ValdiTest
