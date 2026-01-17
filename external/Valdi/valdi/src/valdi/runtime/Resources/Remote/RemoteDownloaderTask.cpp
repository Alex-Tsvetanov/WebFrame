//
//  RemoteDownloaderTask.cpp
//  valdi-ios
//
//  Created by Simon Corsin on 8/30/19.
//

#include "valdi/runtime/Resources/Remote/RemoteDownloaderTask.hpp"
#include "valdi/runtime/Resources/Remote/IRemoteDownloaderItemHandler.hpp"

namespace Valdi {

RemoteDownloaderTask::RemoteDownloaderTask(StringBox localFilename,
                                           StringBox url,
                                           BytesView expectedHash,
                                           const IRemoteDownloaderItemHandler& itemHandler)
    : _localFilename(std::move(localFilename)),
      _url(std::move(url)),
      _expectedHash(std::move(expectedHash)),
      _itemHandler(itemHandler) {
    _sw.start();
}

void RemoteDownloaderTask::appendRequest(RemoteDownloaderRequest request) {
    _requests.emplace_back(std::move(request));
}

void RemoteDownloaderTask::prependRequest(RemoteDownloaderRequest request) {
    _requests.emplace(_requests.begin(), std::move(request));
}

const std::vector<RemoteDownloaderRequest>& RemoteDownloaderTask::getRequests() const {
    return _requests;
}

void RemoteDownloaderTask::clearRequests() {
    _requests.clear();
}

const StringBox& RemoteDownloaderTask::getLocalFilename() const {
    return _localFilename;
}

const StringBox& RemoteDownloaderTask::getUrl() const {
    return _url;
}

const BytesView& RemoteDownloaderTask::getExpectedHash() const {
    return _expectedHash;
}

std::string RemoteDownloaderTask::getItemDescription() const {
    return fmt::format("{} '{}'", _itemHandler.getItemTypeDescription(), _localFilename.toStringView());
}

snap::utils::time::Duration<std::chrono::steady_clock> RemoteDownloaderTask::getElapsedTime() const {
    return _sw.elapsed();
}

const IRemoteDownloaderItemHandler& RemoteDownloaderTask::getItemHandler() const {
    return _itemHandler;
}

} // namespace Valdi
