//
//  RemoteDownloaderRequest.hpp
//  valdi-ios
//
//  Created by Simon Corsin on 8/30/19.
//

#pragma once

#include "valdi_core/cpp/Utils/Result.hpp"
#include "valdi_core/cpp/Utils/Value.hpp"

#include "valdi_core/cpp/Utils/Function.hpp"

namespace Valdi {

enum RemoteDownloaderLoadSource {
    RemoteDownloaderLoadSourceInMemory,
    RemoteDownloaderLoadSourceDiskCache,
    RemoteDownloaderLoadSourceNetwork
};

using RemoteDownloaderLoadCompletion = Function<void(Result<Value>, RemoteDownloaderLoadSource)>;

class RemoteDownloaderRequest {
public:
    explicit RemoteDownloaderRequest(RemoteDownloaderLoadCompletion&& completion);

    const RemoteDownloaderLoadCompletion& getCompletion() const;

private:
    RemoteDownloaderLoadCompletion _completion;
};

} // namespace Valdi
