//
//  RemoteDownloaderTask.hpp
//  valdi-ios
//
//  Created by Simon Corsin on 8/30/19.
//

#pragma once

#include "valdi/runtime/Resources/Remote/RemoteDownloaderRequest.hpp"
#include "valdi_core/cpp/Utils/Bytes.hpp"
#include "valdi_core/cpp/Utils/StringBox.hpp"

#include "utils/time/StopWatch.hpp"

#include <vector>

namespace Valdi {

class IRemoteDownloaderItemHandler;

class RemoteDownloaderTask {
public:
    RemoteDownloaderTask(StringBox localFilename,
                         StringBox url,
                         BytesView expectedHash,
                         const IRemoteDownloaderItemHandler& itemHandler);

    // NOTE: All non const methods are NOT thread safe
    void appendRequest(RemoteDownloaderRequest request);
    void prependRequest(RemoteDownloaderRequest request);

    const std::vector<RemoteDownloaderRequest>& getRequests() const;
    void clearRequests();

    const StringBox& getLocalFilename() const;
    const StringBox& getUrl() const;
    const BytesView& getExpectedHash() const;

    std::string getItemDescription() const;

    snap::utils::time::Duration<std::chrono::steady_clock> getElapsedTime() const;
    const IRemoteDownloaderItemHandler& getItemHandler() const;

private:
    StringBox _localFilename;
    StringBox _url;
    BytesView _expectedHash;
    const IRemoteDownloaderItemHandler& _itemHandler;
    std::vector<RemoteDownloaderRequest> _requests;
    snap::utils::time::StopWatch _sw;
};

} // namespace Valdi
