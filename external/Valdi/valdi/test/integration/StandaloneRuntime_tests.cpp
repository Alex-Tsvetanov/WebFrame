#include "valdi/standalone_runtime/StandaloneExitCoordinator.hpp"
#include "valdi/standalone_runtime/StandaloneMainQueue.hpp"

#include "valdi/jsbridge/JavaScriptBridge.hpp"

#include "valdi/runtime/Utils/AsyncGroup.hpp"
#include "valdi_core/cpp/Utils/ByteBuffer.hpp"
#include "valdi_core/cpp/Utils/ConsoleLogger.hpp"

#include "valdi_test_utils.hpp"

#include <gtest/gtest.h>

using namespace Valdi;
using namespace snap::valdi;

namespace ValdiTest {

class TestQueueListener : public IQueueListener {
public:
    TestQueueListener(const Ref<AsyncGroup>& group, const Shared<IQueueListener>&& listener)
        : _group(group), _listener(std::move(listener)) {}
    ~TestQueueListener() override = default;

    void onQueueNonEmpty() override {
        _group->enter();
        _listener->onQueueNonEmpty();
    }

    void onQueueEmpty() override {
        _listener->onQueueEmpty();
        _group->leave();
    }

private:
    Ref<AsyncGroup> _group;
    Shared<IQueueListener> _listener;
};

TEST(StandaloneRuntime, exitCoordinatorStopsMainQueueWhenAllQueuesAreEmpty) {
    auto mainQueue = makeShared<MainQueue>();
    auto jsQueue = DispatchQueue::create(STRING_LITERAL("Valdi JS Queue"), ThreadQoSClassMax);

    auto exitCoordinator = makeShared<StandaloneExitCoordinator>(jsQueue, mainQueue);
    exitCoordinator->postInit();
    exitCoordinator->setEnabled(true);

    auto group = makeShared<AsyncGroup>();
    jsQueue->setListener(makeShared<TestQueueListener>(group, jsQueue->getListener()));
    mainQueue->setListener(makeShared<TestQueueListener>(group, mainQueue->getListener()));

    int ranTasksCount = 0;
    int expectedTasksCount = 5;

    for (int i = 0; i < expectedTasksCount; i++) {
        jsQueue->async([&]() { mainQueue->enqueue([&]() { ranTasksCount++; }); });
    }

    ASSERT_FALSE(mainQueue->isDisposed());

    mainQueue->runUntilTrue([&]() { return ranTasksCount == expectedTasksCount; });

    mainQueue->flush();

    ASSERT_TRUE(group->blockingWaitWithTimeout(std::chrono::seconds(5)));

    exitCoordinator->flushUpdatesSync();

    // The main queue should have been disposed as all the JS tasks and main queue tasks have been flushed

    ASSERT_TRUE(mainQueue->isDisposed());
}

TEST(StandaloneRuntime, canPrecompileJs) {
    auto jsContent = makeShared<ByteBuffer>("module.exports.hello = 42;");

    auto result =
        ValdiStandaloneRuntime::preCompile(JavaScriptBridge::get(snap::valdi_core::JavaScriptEngineType::QuickJS),
                                           jsContent->toBytesView(),
                                           STRING_LITERAL("filename.js"));

    ASSERT_TRUE(result) << result.description();

    auto data = result.value();

    // We don't explicitly check here that the precompiled JS can run in the engine, this test is
    // done in the JSIntegrationTests. Here we just verify that we got a success result with some output.
    ASSERT_TRUE(data.size() > 32);
}

} // namespace ValdiTest
