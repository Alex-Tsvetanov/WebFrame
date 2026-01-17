//
//  MockAssetLoader.cpp
//  valdi-pc
//
//  Created by Ramzy Jaber on 3/17/22.
//

#include "MockAssetLoader.hpp"

#include "valdi_core/cpp/Utils/ContainerUtils.hpp"
#include "valdi_test_utils.hpp"

class MockAssetLoader;

MockAssetLoaderCancelable::MockAssetLoaderCancelable(MockAssetLoader& assetLoader,
                                                     const Weak<AssetLoaderCompletion>& completion)
    : _assetLoader(assetLoader), _completion(completion) {}

void MockAssetLoaderCancelable::cancel() {
    auto completion = _completion.lock();

    if (completion != nullptr) {
        _assetLoader.cancelRequest(strongSmallRef(completion.get()));
    }
}

MockAssetLoader::MockAssetLoader()
    : MockAssetLoader(
          {STRING_LITERAL("asset"), STRING_LITERAL("file"), STRING_LITERAL("http"), STRING_LITERAL("https")},
          snap::valdi_core::AssetOutputType::Dummy) {}

MockAssetLoader::MockAssetLoader(std::vector<Valdi::StringBox>&& urlSchemes,
                                 snap::valdi_core::AssetOutputType outputType)
    : Valdi::AssetLoader(std::move(urlSchemes)), _outputType(outputType) {}

snap::valdi_core::AssetOutputType MockAssetLoader::getOutputType() const {
    return _outputType;
}

void MockAssetLoader::setAssetResponse(const StringBox& url, const Ref<Valdi::LoadedAsset>& response) {
    setAssetResponse(url, Result(response));
}

void MockAssetLoader::setAssetResponse(const StringBox& url, const Result<Ref<Valdi::LoadedAsset>>& response) {
    setAssetResponseCallback(url, [response]() { return response; });
}

void MockAssetLoader::setAssetResponseCallback(const StringBox& url, AssetResponseCallback callback) {
    std::lock_guard<Mutex> lock(_mutex);
    _responses[url] = std::move(callback);
}

void MockAssetLoader::updateAllAssets(const Ref<Valdi::LoadedAsset>& asset) {
    std::vector<Weak<Valdi::AssetLoaderCompletion>> completions;
    {
        std::lock_guard<Mutex> lock(_mutex);
        completions = _completions;
    }

    for (const auto& it : completions) {
        auto completion = it.lock();
        if (completion != nullptr) {
            completion->onLoadComplete(asset);
        }
    }
}

Valdi::Result<Valdi::Value> MockAssetLoader::requestPayloadFromURL(const Valdi::StringBox& url) {
    {
        std::lock_guard<Mutex> lock(_mutex);
        _requestPayloadCount++;

        if (_payloadError) {
            return _payloadError.value();
        }
    }

    return Valdi::Value(url);
}

Shared<snap::valdi_core::Cancelable> MockAssetLoader::loadAsset(const Value& requestPayload,
                                                                int32_t preferredWidth,
                                                                int32_t preferredHeight,
                                                                const Value& /*filter*/,
                                                                const Ref<AssetLoaderCompletion>& completion) {
    auto url = requestPayload.toStringBox();
    Result<Ref<Valdi::LoadedAsset>> result;

    {
        std::lock_guard<Mutex> lock(_mutex);

        const auto& it = _responses.find(url);
        if (it != _responses.end()) {
            result = it->second();
        } else {
            result = Error(STRING_FORMAT("No response registered for identifier '{}'", url));
        }

        _completions.emplace_back(completion);
        _loadAssetCount++;
    }

    if (_workerQueue != nullptr) {
        _workerQueue->enqueue([this, completion, result]() { handleResponse(completion, result); });
    } else {
        handleResponse(completion, result);
    }

    return makeShared<MockAssetLoaderCancelable>(*this, completion);
}

void MockAssetLoader::cancelRequest(const Ref<AssetLoaderCompletion>& completion) {
    std::lock_guard<Mutex> lock(_mutex);

    Valdi::eraseFirstIf(_completions, [&](const auto& it) { return it.lock().get() == completion.get(); });
}

void MockAssetLoader::setWorkerQueue(const Ref<TaskQueue>& workerQueue) {
    _workerQueue = workerQueue;
}

size_t MockAssetLoader::getCurrentLoadRequestsCount() const {
    std::lock_guard<Mutex> lock(_mutex);
    return _completions.size();
}

size_t MockAssetLoader::getLoadAssetCount() const {
    std::lock_guard<Mutex> lock(_mutex);
    return _loadAssetCount;
}

size_t MockAssetLoader::getRequestPayloadCallCount() const {
    std::lock_guard<Mutex> lock(_mutex);
    return _requestPayloadCount;
}

void MockAssetLoader::setRequestPayloadError(const Error& error) {
    std::lock_guard<Mutex> lock(_mutex);
    _payloadError = {error};
}

void MockAssetLoader::handleResponse(const Ref<AssetLoaderCompletion>& completion,
                                     const Result<Ref<Valdi::LoadedAsset>>& result) {
    {
        std::lock_guard<Mutex> lock(_mutex);

        bool foundCompletion = false;
        for (const auto& it : _completions) {
            if (it.lock() == completion) {
                foundCompletion = true;
                break;
            }
        }

        if (!foundCompletion) {
            return;
        }
    }

    completion->onLoadComplete(result);
}
