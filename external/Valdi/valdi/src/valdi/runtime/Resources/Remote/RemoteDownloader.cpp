//
//  RemoteDownloader.cpp
//  valdi-ios
//
//  Created by Simon Corsin on 8/30/19.
//

#include "valdi/runtime/Resources/Remote/RemoteDownloader.hpp"

#include "valdi/runtime/Resources/Remote/RemoteDownloaderTask.hpp"

#include "valdi/runtime/Interfaces/IDiskCache.hpp"
#include "valdi_core/cpp/Threading/DispatchQueue.hpp"

#include "valdi/runtime/Utils/AsyncGroup.hpp"
#include "valdi/runtime/Utils/BytesUtils.hpp"
#include "valdi/runtime/Utils/HTTPRequestManagerUtils.hpp"
#include "valdi_core/cpp/Resources/ValdiArchive.hpp"
#include "valdi_core/cpp/Utils/LoggerUtils.hpp"
#include "valdi_core/cpp/Utils/StringCache.hpp"
#include "valdi_core/cpp/Utils/ValdiObject.hpp"

#include "utils/encoding/Base64Utils.hpp"

#include "valdi_core/Cancelable.hpp"
#include "valdi_core/HTTPRequest.hpp"
#include "valdi_core/HTTPRequestManager.hpp"
#include "valdi_core/HTTPRequestManagerCompletion.hpp"
#include "valdi_core/HTTPResponse.hpp"
#include "valdi_core/cpp/Utils/ValueTypedArray.hpp"

#include "utils/debugging/Assert.hpp"

namespace Valdi {

STRING_CONST(urlFilePath, "url")
STRING_CONST(dataFilePath, "data")

class CachedBundleItem {
public:
    CachedBundleItem(StringBox url, BytesView data) : _url(std::move(url)), _data(std::move(data)) {}

    Ref<ByteBuffer> serialize() const {
        ValdiArchiveBuilder builder;
        builder.addEntry(ValdiArchiveEntry(urlFilePath(), _url));
        builder.addEntry(ValdiArchiveEntry(dataFilePath(), _data.data(), _data.size()));

        return builder.build();
    }

    const StringBox& getUrl() const {
        return _url;
    }

    const BytesView& getData() const {
        return _data;
    }

    static Result<CachedBundleItem> deserialize(const BytesView& serializedData) {
        auto module = ValdiArchive(serializedData.begin(), serializedData.end());

        StringBox url;
        BytesView data;
        auto urlCount = 0;
        auto dataCount = 0;

        auto entries = module.getEntries();
        if (!entries) {
            return entries.moveError();
        }

        for (const auto& entry : entries.value()) {
            if (entry.filePath == urlFilePath()) {
                url = entry.getStringData();
                urlCount++;
            } else if (entry.filePath == dataFilePath()) {
                data = BytesView(serializedData.getSource(), entry.data, entry.dataLength);
                dataCount++;
            }
        }

        if (urlCount != 1 || dataCount != 1) {
            return Error("Invalid cached bundle item");
        }

        return CachedBundleItem(std::move(url), std::move(data));
    }

private:
    StringBox _url;
    BytesView _data;
};

CachedLoadedItem::CachedLoadedItem() = default;

CachedLoadedItem::CachedLoadedItem(const Value& value) : _value(value) {
    markAccessed();
}

CachedLoadedItem::~CachedLoadedItem() = default;

void CachedLoadedItem::markAccessed() {
    _lastAccessTime = std::chrono::steady_clock::now();
}

const Value& CachedLoadedItem::getValue() const {
    return _value;
}

bool CachedLoadedItem::isExpired(const std::chrono::steady_clock::time_point& expirationTime) const {
    if (_lastAccessTime > expirationTime) {
        return false;
    }

    auto object = _value.getValdiObject();
    if (object == nullptr) {
        return true;
    }

    return object.use_count() <= 2;
}

RemoteDownloader::RemoteDownloader(const Ref<IDiskCache>& diskCache,
                                   const Holder<Shared<snap::valdi_core::HTTPRequestManager>>& requestManager,
                                   const Ref<DispatchQueue>& workerQueue,
                                   ILogger& logger)
    : _diskCache(diskCache), _requestManager(requestManager), _workQueue(workerQueue), _logger(logger) {}

RemoteDownloader::~RemoteDownloader() = default;

bool RemoteDownloader::loadFromDiskCache(const Shared<RemoteDownloaderTask>& task) {
    if (_diskCache == nullptr) {
        return false;
    }

    auto result = _diskCache->load(Path(task->getLocalFilename()));
    if (!result) {
        return false;
    }

    auto data = result.moveValue();

    auto cachedBundleItemResult = CachedBundleItem::deserialize(data);
    if (!cachedBundleItemResult) {
        VALDI_ERROR(_logger,
                    "Failed to load module from disk cache at {}: {}",
                    task->getLocalFilename(),
                    cachedBundleItemResult.error());
        return false;
    }

    auto cachedBundleItem = cachedBundleItemResult.moveValue();

    if (cachedBundleItem.getUrl() != task->getUrl()) {
        // The url changed we need to re-download the module
        VALDI_INFO(_logger, "Module from disk cache at {} is out of date, need re-download", task->getLocalFilename());
        return false;
    }

    auto transformedResult = transformLoadResult(task, cachedBundleItem.getData());
    if (!transformedResult) {
        VALDI_WARN(_logger,
                   "Module from disk cache at {} is corrupted, need re-download: {}",
                   task->getLocalFilename(),
                   transformedResult.error());
        return false;
    }

    // Our cached bundle item is correct, we can mark the load as completed
    loadCompleted(task, transformedResult.moveValue(), true);

    return true;
}

void RemoteDownloader::loadFileUrl(const Shared<RemoteDownloaderTask>& task) {
    // URL is expected to be in the format file://
    if (_diskCache == nullptr) {
        loadCompleted(task, Error("Cannot load a file URL without a DiskCache"), true);
        return;
    }

    auto absolutePath = task->getUrl().substring(std::string_view("file://").size());

    auto loadResult = _diskCache->load(Path(absolutePath));
    if (!loadResult) {
        loadCompleted(task, loadResult.moveError(), true);
        return;
    }

    auto transformedResult = transformLoadResult(task, loadResult.value());
    loadCompleted(task, transformedResult, true);
}

void RemoteDownloader::loadBase64Data(const Shared<RemoteDownloaderTask>& task) {
    std::string_view prefixToFind = ";base64,";

    auto index = task->getUrl().find(prefixToFind);
    if (!index) {
        loadCompleted(task, Error("Invalid base64"), true);
        return;
    }

    auto substrIndex = index.value() + prefixToFind.length();

    auto strView = std::string_view(task->getUrl().getCStr() + substrIndex, task->getUrl().length() - substrIndex);

    auto out = makeShared<Bytes>();

    if (!snap::utils::encoding::base64ToBinary(strView, *out)) {
        loadCompleted(task, Error("Invalid base64"), true);
        return;
    }

    auto transformedResult = transformLoadResult(task, BytesView(out));
    loadCompleted(task, transformedResult, true);
}

void RemoteDownloader::storeDownloadedItemInDiskCache(const Shared<RemoteDownloaderTask>& task, const BytesView& data) {
    if (_diskCache == nullptr) {
        return;
    }

    VALDI_INFO(_logger,
               "Storing downloaded {} from {} in disk cache at {}",
               task->getItemDescription(),
               task->getUrl(),
               task->getLocalFilename());

    CachedBundleItem bundleItem(task->getUrl(), data);

    auto serializedFile = bundleItem.serialize();

    auto bytesView = serializedFile->toBytesView();
    auto result = _diskCache->store(Path(task->getLocalFilename()), bytesView);
    if (!result) {
        VALDI_INFO(_logger,
                   "Failed to store downloaded {} from {} in disk cache at {}: {}",
                   task->getItemDescription(),
                   task->getUrl(),
                   task->getLocalFilename(),
                   result.error());
    }
}

void RemoteDownloader::handleSuccessRemoteDownload(const Shared<RemoteDownloaderTask>& task,
                                                   const BytesView& downloadedPayload) {
    auto preprocessedPayload = task->getItemHandler().preprocess(task->getLocalFilename(), downloadedPayload);
    if (!preprocessedPayload) {
        loadCompleted(task, preprocessedPayload.error().rethrow("Failed to preprocess downloaded item"), false);
        return;
    }

    auto weakThis = weak_from_this();
    _workQueue->async([=]() {
        auto strongThis = weakThis.lock();
        if (strongThis != nullptr) {
            strongThis->storeDownloadedItemInDiskCache(task, preprocessedPayload.value());
        }
    });

    loadCompleted(task, transformLoadResult(task, preprocessedPayload.value()), false);
}

void RemoteDownloader::remoteResponseReceived(const Shared<RemoteDownloaderTask>& task,
                                              Result<snap::valdi_core::HTTPResponse> responseResult) {
    if (!responseResult) {
        loadCompleted(task, responseResult.error().rethrow("Remote load failed"), false);
        return;
    }

    auto response = responseResult.moveValue();

    if (response.statusCode < 200 || response.statusCode >= 300) {
        loadCompleted(
            task, Error(STRING_FORMAT("Remote load failed: Got HTTP status code {}", response.statusCode)), false);
        return;
    }

    BytesView bodyBytes;

    const auto& body = response.body;
    if (body) {
        bodyBytes = *body;
    }

    if (bodyBytes.empty()) {
        loadCompleted(task, Error(STRING_LITERAL("Remote load failed: Got empty HTTP body")), false);
        return;
    }

    auto expectedHash = task->getExpectedHash();
    if (!expectedHash.empty()) {
        auto calculatedHash = BytesUtils::sha256(bodyBytes)->toBytesView();
        bool hashMatches = expectedHash == calculatedHash;
        if (!hashMatches) {
            loadCompleted(
                task,
                Error(STRING_FORMAT("Remote load failed: SHA256 integrity check failed. Expected '{}', got '{}'",
                                    expectedHash.asStringView(),
                                    calculatedHash.asStringView())),
                false);
            return;
        }
    }

    auto weakThis = weak_from_this();
    _workQueue->async([=]() {
        auto strongThis = weakThis.lock();
        if (strongThis == nullptr) {
            return;
        }

        strongThis->handleSuccessRemoteDownload(task, bodyBytes);
    });
}

void RemoteDownloader::loadRemote(const Shared<RemoteDownloaderTask>& task) {
    auto requestManager = getRequestManager();

    if (requestManager == nullptr) {
        loadCompleted(task, Error(STRING_LITERAL("No RequestManager set in the RemoteDownloader")), false);
        return;
    }

    int32_t priority = 4;

    static auto kGetMethod = STRING_LITERAL("GET");

    snap::valdi_core::HTTPRequest request(task->getUrl(), kGetMethod, Value(), {}, priority);

    VALDI_INFO(_logger, "Starting load of {} at {}", task->getItemDescription(), task->getUrl());

    auto weakThis = weak_from_this();
    requestManager->performRequest(
        request, HTTPRequestManagerUtils::makeRequestCompletion([=](Result<snap::valdi_core::HTTPResponse> response) {
            auto strongThis = weakThis.lock();
            if (strongThis == nullptr) {
                return;
            }

            strongThis->remoteResponseReceived(task, std::move(response));
        }));
}

void RemoteDownloader::doLoad(const Shared<RemoteDownloaderTask>& task) {
    if (task->getUrl().hasPrefix("file://")) {
        loadFileUrl(task);
    } else if (task->getUrl().hasPrefix("data:image/")) {
        loadBase64Data(task);
    } else if (!loadFromDiskCache(task)) {
        loadRemote(task);
    }
}

void RemoteDownloader::removeItem(const StringBox& url) {
    std::lock_guard<Mutex> guard(_mutex);

    auto it = _downloadedItemByUrl.find(url);
    if (it != _downloadedItemByUrl.end()) {
        _downloadedItemByUrl.erase(it);
    }
}

void RemoteDownloader::enqueue(const StringBox& localFilename,
                               const StringBox& url,
                               const IRemoteDownloaderItemHandler& itemHandler,
                               const BytesView& expectedHash,
                               RemoteDownloaderLoadCompletion completion) {
    std::unique_lock<Mutex> guard(_mutex);

    const auto& inMemoryIt = _downloadedItemByUrl.find(url);
    if (inMemoryIt != _downloadedItemByUrl.end()) {
        inMemoryIt->second.markAccessed();

        auto value = inMemoryIt->second.getValue();
        guard.unlock();

        completion(std::move(value), RemoteDownloaderLoadSourceInMemory);
        return;
    }

    auto shouldStartRequest = false;

    Shared<RemoteDownloaderTask> task;

    const auto& it = _taskByUrl.find(url);
    if (it == _taskByUrl.end()) {
        task = Valdi::makeShared<RemoteDownloaderTask>(localFilename, url, expectedHash, itemHandler);
        _taskByUrl[url] = task;

        shouldStartRequest = true;
    } else {
        task = it->second;
    }

    task->appendRequest(RemoteDownloaderRequest(std::move(completion)));

    if (shouldStartRequest) {
        guard.unlock();

        auto weakThis = weak_from_this();
        _workQueue->async([=]() {
            auto strongThis = weakThis.lock();
            if (strongThis != nullptr) {
                strongThis->doLoad(task);
            }
        });
    }
}

Result<Value> RemoteDownloader::transformLoadResult(const Shared<RemoteDownloaderTask>& task, const BytesView& result) {
    auto transformResult = task->getItemHandler().transform(task->getLocalFilename(), result);
    if (!transformResult) {
        return transformResult.error().rethrow("Failed to transform result");
    }

    return transformResult;
}

void RemoteDownloader::loadCompleted(const Shared<RemoteDownloaderTask>& task,
                                     const Result<Value>& transformedResult,
                                     bool fromDiskCache) {
    [[maybe_unused]] auto elapsed = task->getElapsedTime();

    if (transformedResult.failure()) {
        VALDI_ERROR(_logger,
                    "Failed to load {} at remote URL {} in {}: {}",
                    task->getItemDescription(),
                    task->getUrl(),
                    elapsed,
                    transformedResult.error());
    }

    auto removedTask = removeTask(task->getUrl(), transformedResult);
    SC_ASSERT_NOTNULL(removedTask);

    for (const auto& request : removedTask->getRequests()) {
        auto loadSource = fromDiskCache ? RemoteDownloaderLoadSourceDiskCache : RemoteDownloaderLoadSourceNetwork;
        request.getCompletion()(transformedResult, loadSource);
    }

    removedTask->clearRequests();
}

Shared<RemoteDownloaderTask> RemoteDownloader::removeTask(const StringBox& url, const Result<Value>& taskResult) {
    std::lock_guard<Mutex> guard(_mutex);

    if (taskResult.success()) {
        _downloadedItemByUrl[url] = CachedLoadedItem(taskResult.value());
    }

    const auto& it = _taskByUrl.find(url);
    if (it == _taskByUrl.end()) {
        return nullptr;
    }

    auto task = it->second;
    _taskByUrl.erase(it);
    return task;
}

std::shared_ptr<snap::valdi_core::HTTPRequestManager> RemoteDownloader::getRequestManager() const {
    return _requestManager.get();
}

} // namespace Valdi
