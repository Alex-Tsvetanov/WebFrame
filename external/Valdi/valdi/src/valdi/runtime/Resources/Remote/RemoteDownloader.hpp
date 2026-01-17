//
//  RemoteDownloader.hpp
//  valdi-ios
//
//  Created by Simon Corsin on 8/30/19.
//

#pragma once

#include "valdi/runtime/Resources/Remote/IRemoteDownloaderItemHandler.hpp"
#include "valdi/runtime/Resources/Remote/RemoteDownloaderRequest.hpp"
#include "valdi_core/cpp/Utils/Bytes.hpp"
#include "valdi_core/cpp/Utils/FlatMap.hpp"
#include "valdi_core/cpp/Utils/Holder.hpp"
#include "valdi_core/cpp/Utils/Mutex.hpp"
#include "valdi_core/cpp/Utils/Result.hpp"
#include "valdi_core/cpp/Utils/Shared.hpp"
#include "valdi_core/cpp/Utils/StringBox.hpp"
#include "valdi_core/cpp/Utils/Value.hpp"

#include "valdi_core/cpp/Utils/Function.hpp"
#include <atomic>
#include <mutex>

namespace snap::valdi_core {
class HTTPRequestManager;
struct HTTPResponse;
} // namespace snap::valdi_core

namespace Valdi {

class ILogger;
class ValdiModuleArchive;
class DispatchQueue;
class RemoteDownloaderTask;
class IDiskCache;

class CachedLoadedItem {
public:
    CachedLoadedItem();
    explicit CachedLoadedItem(const Value& value);
    ~CachedLoadedItem();

    const Value& getValue() const;
    void markAccessed();

    bool isExpired(const std::chrono::steady_clock::time_point& expirationTime) const;

private:
    Value _value;
    std::chrono::steady_clock::time_point _lastAccessTime;
};

/**
 RemoteDownloader manages download of remote artifacts.
 It manages an in-memory cache, a disk cache and hit the network if needed.
 An artifact is identified by a local filename. The cache key is the url.
 A cached file on disk is considered out of date when the given url is not the
 same as the one that was used to fetch it. As such the design of this class is
 a bit different than conventional resource downloaders: it is designed to manage
 a limited set of unique file ids which can change over time, as opposed to
 manage an unbounded amount of temporary files identified by their urls. The disk
 cache thus only grows when new modules are added in the app.
 */
class RemoteDownloader : public std::enable_shared_from_this<RemoteDownloader> {
public:
    RemoteDownloader(const Ref<IDiskCache>& diskCache,
                     const Holder<Shared<snap::valdi_core::HTTPRequestManager>>& requestManager,
                     const Ref<DispatchQueue>& workerQueue,
                     ILogger& logger);
    virtual ~RemoteDownloader();

    void enqueue(const StringBox& localFilename,
                 const StringBox& url,
                 const IRemoteDownloaderItemHandler& itemHandler,
                 const BytesView& expectedHash,
                 RemoteDownloaderLoadCompletion completion);

    std::shared_ptr<snap::valdi_core::HTTPRequestManager> getRequestManager() const;

    void removeItem(const StringBox& url);

private:
    Ref<IDiskCache> _diskCache;
    mutable Mutex _mutex;
    const Holder<Shared<snap::valdi_core::HTTPRequestManager>> _requestManager;
    Ref<DispatchQueue> _workQueue;
    [[maybe_unused]] ILogger& _logger;

    FlatMap<StringBox, Shared<RemoteDownloaderTask>> _taskByUrl;
    FlatMap<StringBox, CachedLoadedItem> _downloadedItemByUrl;

    void doLoad(const Shared<RemoteDownloaderTask>& task);

    bool loadFromDiskCache(const Shared<RemoteDownloaderTask>& task);
    void loadRemote(const Shared<RemoteDownloaderTask>& task);
    void loadFileUrl(const Shared<RemoteDownloaderTask>& task);
    void loadBase64Data(const Shared<RemoteDownloaderTask>& task);

    void loadCompleted(const Shared<RemoteDownloaderTask>& task,
                       const Result<Value>& transformedResult,
                       bool fromDiskCache);

    void remoteResponseReceived(const Shared<RemoteDownloaderTask>& task,
                                Result<snap::valdi_core::HTTPResponse> responseResult);
    void handleSuccessRemoteDownload(const Shared<RemoteDownloaderTask>& task, const BytesView& downloadedPayload);
    void storeDownloadedItemInDiskCache(const Shared<RemoteDownloaderTask>& task, const BytesView& data);
    BytesView loadDownloadedItemFromDiskCache(const Shared<RemoteDownloaderTask>& task);

    Shared<RemoteDownloaderTask> removeTask(const StringBox& url, const Result<Value>& taskResult);

    static Result<Value> transformLoadResult(const Shared<RemoteDownloaderTask>& task, const BytesView& result);
};

} // namespace Valdi
