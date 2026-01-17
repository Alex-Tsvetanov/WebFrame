//
//  RemoteDownloaderRequest.cpp
//  valdi-ios
//
//  Created by Simon Corsin on 8/30/19.
//

#include "valdi/runtime/Resources/Remote/RemoteDownloaderRequest.hpp"

namespace Valdi {

RemoteDownloaderRequest::RemoteDownloaderRequest(RemoteDownloaderLoadCompletion&& completion)
    : _completion(std::move(completion)) {}

const RemoteDownloaderLoadCompletion& RemoteDownloaderRequest::getCompletion() const {
    return _completion;
}

} // namespace Valdi
