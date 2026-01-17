//
//  MockAssetLoader.hpp
//  valdi-pc
//
//  Created by Ramzy Jaber on 3/17/22.
//

#pragma once

#include "valdi/runtime/Resources/AssetLoader.hpp"
#include "valdi/runtime/Resources/AssetLoaderCompletion.hpp"
#include "valdi_core/cpp/Resources/LoadedAsset.hpp"
#include "valdi_core/cpp/Threading/TaskQueue.hpp"
#include "valdi_core/cpp/Utils/FlatMap.hpp"
#include "valdi_core/cpp/Utils/Function.hpp"
#include "valdi_core/cpp/Utils/Result.hpp"
#include "valdi_core/cpp/Utils/StringBox.hpp"

#include "RequestManagerMock.hpp"

class MockAssetLoader;

using AssetResponseCallback = std::function<Valdi::Result<Valdi::Ref<Valdi::LoadedAsset>>()>;

class MockAssetLoaderCancelable : public snap::valdi_core::Cancelable {
public:
    MockAssetLoaderCancelable(MockAssetLoader& assetLoader, const Weak<AssetLoaderCompletion>& completion);

    void cancel() override;

private:
    MockAssetLoader& _assetLoader;
    Weak<AssetLoaderCompletion> _completion;
};

class MockAssetLoader : public Valdi::AssetLoader {
public:
    MockAssetLoader();
    MockAssetLoader(std::vector<Valdi::StringBox>&& urlSchemes, snap::valdi_core::AssetOutputType outputType);

    snap::valdi_core::AssetOutputType getOutputType() const override;

    void setAssetResponse(const StringBox& url, const Ref<Valdi::LoadedAsset>& response);

    void setAssetResponse(const StringBox& url, const Result<Ref<Valdi::LoadedAsset>>& response);

    void setAssetResponseCallback(const StringBox& url, AssetResponseCallback callback);

    void updateAllAssets(const Ref<Valdi::LoadedAsset>& asset);

    Valdi::Result<Valdi::Value> requestPayloadFromURL(const Valdi::StringBox& url) override;

    Shared<snap::valdi_core::Cancelable> loadAsset(const Value& requestPayload,
                                                   int32_t preferredWidth,
                                                   int32_t preferredHeight,
                                                   const Value& filter,
                                                   const Ref<AssetLoaderCompletion>& completion) override;

    void cancelRequest(const Ref<AssetLoaderCompletion>& completion);

    void setWorkerQueue(const Ref<TaskQueue>& workerQueue);

    size_t getCurrentLoadRequestsCount() const;

    size_t getLoadAssetCount() const;

    size_t getRequestPayloadCallCount() const;

    void setRequestPayloadError(const Error& error);

private:
    snap::valdi_core::AssetOutputType _outputType;
    mutable Mutex _mutex;
    FlatMap<StringBox, AssetResponseCallback> _responses;
    std::vector<Valdi::Weak<Valdi::AssetLoaderCompletion>> _completions;
    Ref<TaskQueue> _workerQueue;
    size_t _requestPayloadCount = 0;
    size_t _loadAssetCount = 0;
    std::optional<Error> _payloadError;

    void handleResponse(const Ref<AssetLoaderCompletion>& weakCompletion,
                        const Result<Ref<Valdi::LoadedAsset>>& result);
};
