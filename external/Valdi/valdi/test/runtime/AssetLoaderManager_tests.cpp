#include "MockAssetLoader.hpp"
#include "MockAssetLoaderFactory.hpp"
#include "MockDownloader.hpp"
#include "RequestManagerMock.hpp"
#include "TestAsyncUtils.hpp"
#include "valdi/runtime/Interfaces/IRemoteDownloader.hpp"
#include "valdi/runtime/Resources/AssetLoaderFactory.hpp"
#include "valdi/runtime/Resources/AssetLoaderManager.hpp"
#include "valdi/runtime/Resources/BytesAssetLoader.hpp"
#include "valdi/standalone_runtime/InMemoryDiskCache.hpp"
#include "valdi_core/cpp/Utils/ConsoleLogger.hpp"
#include <gtest/gtest.h>

using namespace Valdi;

namespace ValdiTest {

class AssetLoaderManagerTests : public ::testing::Test {
protected:
    void SetUp() override {
        _assetLoaderManager = makeShared<AssetLoaderManager>();
    }

    void TearDown() override {
        _assetLoaderManager = nullptr;
        if (_dispatchQueue != nullptr) {
            _dispatchQueue->fullTeardown();
            _dispatchQueue = nullptr;
        }
    }

    Ref<MockAssetLoader> appendAssetLoader(std::vector<StringBox>&& urlSchemes,
                                           snap::valdi_core::AssetOutputType outputType) {
        auto assetLoader = makeShared<MockAssetLoader>(std::move(urlSchemes), outputType);
        _assetLoaderManager->registerAssetLoader(assetLoader);

        return assetLoader;
    }

    Ref<MockAssetLoaderFactory> appendAssetLoaderFactory(snap::valdi_core::AssetOutputType outputType) {
        auto factory = makeShared<MockAssetLoaderFactory>(outputType);
        _assetLoaderManager->registerAssetLoaderFactory(factory);

        return factory;
    }

    Ref<AssetLoaderManager> _assetLoaderManager;
    Ref<DispatchQueue> _dispatchQueue;
};

TEST_F(AssetLoaderManagerTests, canResolveAssetLoader) {
    Ref<AssetLoader> assetLoaderBytes =
        appendAssetLoader({STRING_LITERAL("http"), STRING_LITERAL("https")}, snap::valdi_core::AssetOutputType::Bytes);
    Ref<AssetLoader> assetLoaderDummy =
        appendAssetLoader({STRING_LITERAL("http")}, snap::valdi_core::AssetOutputType::Dummy);
    Ref<AssetLoader> assetLoaderAndroid =
        appendAssetLoader({STRING_LITERAL("res")}, snap::valdi_core::AssetOutputType::ImageAndroid);

    ASSERT_EQ(
        assetLoaderBytes,
        _assetLoaderManager->resolveAssetLoader(STRING_LITERAL("http"), snap::valdi_core::AssetOutputType::Bytes));
    ASSERT_EQ(
        assetLoaderBytes,
        _assetLoaderManager->resolveAssetLoader(STRING_LITERAL("https"), snap::valdi_core::AssetOutputType::Bytes));
    ASSERT_EQ(nullptr,
              _assetLoaderManager->resolveAssetLoader(STRING_LITERAL("res"), snap::valdi_core::AssetOutputType::Bytes));
    ASSERT_EQ(
        assetLoaderDummy,
        _assetLoaderManager->resolveAssetLoader(STRING_LITERAL("http"), snap::valdi_core::AssetOutputType::Dummy));
    ASSERT_EQ(
        nullptr,
        _assetLoaderManager->resolveAssetLoader(STRING_LITERAL("https"), snap::valdi_core::AssetOutputType::Dummy));
    ASSERT_EQ(assetLoaderAndroid,
              _assetLoaderManager->resolveAssetLoader(STRING_LITERAL("res"),
                                                      snap::valdi_core::AssetOutputType::ImageAndroid));
    ASSERT_EQ(nullptr,
              _assetLoaderManager->resolveAssetLoader(STRING_LITERAL("http"),
                                                      snap::valdi_core::AssetOutputType::ImageAndroid));
}

TEST_F(AssetLoaderManagerTests, canUnregisterAssetLoader) {
    Ref<AssetLoader> assetLoaderBytes =
        appendAssetLoader({STRING_LITERAL("http"), STRING_LITERAL("https")}, snap::valdi_core::AssetOutputType::Bytes);
    Ref<AssetLoader> assetLoaderDummy =
        appendAssetLoader({STRING_LITERAL("http")}, snap::valdi_core::AssetOutputType::Dummy);

    ASSERT_EQ(
        assetLoaderBytes,
        _assetLoaderManager->resolveAssetLoader(STRING_LITERAL("http"), snap::valdi_core::AssetOutputType::Bytes));
    _assetLoaderManager->unregisterAssetLoader(assetLoaderBytes);
    ASSERT_EQ(
        nullptr,
        _assetLoaderManager->resolveAssetLoader(STRING_LITERAL("http"), snap::valdi_core::AssetOutputType::Bytes));
    ASSERT_EQ(
        assetLoaderDummy,
        _assetLoaderManager->resolveAssetLoader(STRING_LITERAL("http"), snap::valdi_core::AssetOutputType::Dummy));
}

TEST_F(AssetLoaderManagerTests, prioritizesAssetLoadersRegisteredLast) {
    Ref<AssetLoader> assetLoaderBytes1 =
        appendAssetLoader({STRING_LITERAL("http")}, snap::valdi_core::AssetOutputType::Bytes);
    Ref<AssetLoader> assetLoaderBytes2 =
        appendAssetLoader({STRING_LITERAL("http")}, snap::valdi_core::AssetOutputType::Bytes);
    Ref<AssetLoader> assetLoaderBytes3 =
        appendAssetLoader({STRING_LITERAL("http")}, snap::valdi_core::AssetOutputType::Bytes);
    Ref<AssetLoader> assetLoaderBytes4 =
        appendAssetLoader({STRING_LITERAL("http")}, snap::valdi_core::AssetOutputType::Bytes);

    ASSERT_EQ(
        assetLoaderBytes4,
        _assetLoaderManager->resolveAssetLoader(STRING_LITERAL("http"), snap::valdi_core::AssetOutputType::Bytes));

    _assetLoaderManager->unregisterAssetLoader(assetLoaderBytes4);

    ASSERT_EQ(
        assetLoaderBytes3,
        _assetLoaderManager->resolveAssetLoader(STRING_LITERAL("http"), snap::valdi_core::AssetOutputType::Bytes));

    _assetLoaderManager->unregisterAssetLoader(assetLoaderBytes2);
    _assetLoaderManager->unregisterAssetLoader(assetLoaderBytes3);

    ASSERT_EQ(
        assetLoaderBytes1,
        _assetLoaderManager->resolveAssetLoader(STRING_LITERAL("http"), snap::valdi_core::AssetOutputType::Bytes));

    _assetLoaderManager->unregisterAssetLoader(assetLoaderBytes1);

    ASSERT_EQ(
        nullptr,
        _assetLoaderManager->resolveAssetLoader(STRING_LITERAL("http"), snap::valdi_core::AssetOutputType::Bytes));
}

TEST_F(AssetLoaderManagerTests, supportsAssetLoaderFactories) {
    Ref<AssetLoader> assetLoaderBytes =
        appendAssetLoader({STRING_LITERAL("http")}, snap::valdi_core::AssetOutputType::Bytes);

    auto assetLoader =
        _assetLoaderManager->resolveAssetLoader(STRING_LITERAL("http"), snap::valdi_core::AssetOutputType::Dummy);

    ASSERT_TRUE(assetLoader == nullptr);

    auto factory = appendAssetLoaderFactory(snap::valdi_core::AssetOutputType::Dummy);

    assetLoader =
        _assetLoaderManager->resolveAssetLoader(STRING_LITERAL("http"), snap::valdi_core::AssetOutputType::Dummy);

    ASSERT_FALSE(assetLoader == nullptr);
    auto castedAssetLoader = castOrNull<MockedAssetLoaderWithDownloader>(assetLoader);

    ASSERT_TRUE(castedAssetLoader != nullptr);

    ASSERT_EQ(snap::valdi_core::AssetOutputType::Dummy, castedAssetLoader->outputType);
    ASSERT_TRUE(castedAssetLoader->downloader != nullptr);
}

TEST_F(AssetLoaderManagerTests, recreatesNewAssetLoaderFromFactoriesWhenNewBytesAssetLoaderAreAvailable) {
    Ref<AssetLoader> assetLoaderBytes1 =
        appendAssetLoader({STRING_LITERAL("http")}, snap::valdi_core::AssetOutputType::Bytes);

    auto factory = appendAssetLoaderFactory(snap::valdi_core::AssetOutputType::Dummy);

    auto assetLoader1 =
        _assetLoaderManager->resolveAssetLoader(STRING_LITERAL("http"), snap::valdi_core::AssetOutputType::Dummy);

    ASSERT_TRUE(assetLoader1 != nullptr);

    Ref<AssetLoader> assetLoaderBytes2 =
        appendAssetLoader({STRING_LITERAL("http")}, snap::valdi_core::AssetOutputType::Bytes);

    auto assetLoader2 =
        _assetLoaderManager->resolveAssetLoader(STRING_LITERAL("http"), snap::valdi_core::AssetOutputType::Dummy);

    ASSERT_TRUE(assetLoader2 != nullptr);
    ASSERT_NE(assetLoader1, assetLoader2);

    _assetLoaderManager->unregisterAssetLoader(assetLoaderBytes2);

    auto assetLoader3 =
        _assetLoaderManager->resolveAssetLoader(STRING_LITERAL("http"), snap::valdi_core::AssetOutputType::Dummy);

    ASSERT_TRUE(assetLoader3 != nullptr);
    ASSERT_NE(assetLoader2, assetLoader3);
    ASSERT_EQ(assetLoader1, assetLoader3);
}

TEST_F(AssetLoaderManagerTests, downloaderPassedThroughAssetLoaderFactoryCanLoadBytesAsset) {
    auto& logger = ConsoleLogger::getLogger();
    _dispatchQueue = DispatchQueue::create(STRING_LITERAL("com.snap.valdi.tests.BytesAssetLoader"), ThreadQoSClassMax);

    Holder<Shared<snap::valdi_core::HTTPRequestManager>> requestManagerHolder;
    auto assetLoaderBytes = makeShared<BytesAssetLoader>(
        Valdi::makeShared<InMemoryDiskCache>(), requestManagerHolder, _dispatchQueue, logger);
    auto requestManager = makeShared<RequestManagerMock>(logger);

    requestManagerHolder.set(requestManager);

    _assetLoaderManager->registerAssetLoader(assetLoaderBytes);

    auto factory = appendAssetLoaderFactory(snap::valdi_core::AssetOutputType::Dummy);
    auto assetLoader = castOrNull<MockedAssetLoaderWithDownloader>(
        _assetLoaderManager->resolveAssetLoader(STRING_LITERAL("http"), snap::valdi_core::AssetOutputType::Dummy));

    ASSERT_TRUE(assetLoader != nullptr && assetLoader->downloader != nullptr);

    auto content = STRING_LITERAL("This is some asset bytes content");
    auto bytesContent =
        BytesView(content.getInternedString(), reinterpret_cast<const Byte*>(content.getCStr()), content.length());

    requestManager->addMockedResponse(
        STRING_LITERAL("https://snapchat.com/ghost.png"), STRING_LITERAL("GET"), bytesContent);

    auto resultHolder = makeShared<ResultHolder<BytesView>>();

    assetLoader->downloader->downloadItem(
        STRING_LITERAL("https://snapchat.com/ghost.png"),
        [completion = resultHolder->makeCompletion()](const auto& result) { completion(result); });

    auto result = resultHolder->waitForResult();

    ASSERT_TRUE(result) << result.description();
    ASSERT_EQ(bytesContent, result.value());

    ASSERT_EQ(static_cast<size_t>(1), requestManager->getAllPerformedTasks().size());
}

TEST_F(AssetLoaderManagerTests, canUsePregisteredDownloader) {
    auto factory = appendAssetLoaderFactory(snap::valdi_core::AssetOutputType::Dummy);
    Ref<IRemoteDownloader> downloader = makeShared<MockDownloader>(nullptr);
    _assetLoaderManager->registerDownloaderForScheme(STRING_LITERAL("my-bytes"), downloader);

    auto assetLoader =
        _assetLoaderManager->resolveAssetLoader(STRING_LITERAL("my-bytes"), snap::valdi_core::AssetOutputType::Dummy);

    ASSERT_TRUE(assetLoader != nullptr);

    auto castedAssetLoader = castOrNull<MockedAssetLoaderWithDownloader>(assetLoader);

    ASSERT_TRUE(castedAssetLoader != nullptr);
    ASSERT_EQ(downloader, castedAssetLoader->downloader);
}

} // namespace ValdiTest
