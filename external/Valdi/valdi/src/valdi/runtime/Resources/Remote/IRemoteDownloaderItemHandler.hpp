//
//  IRemoteDownloaderItemHandler.hpp
//  valdi-desktop-apple
//
//  Created by Simon Corsin on 3/23/22.
//

#pragma once

#include "valdi_core/cpp/Utils/Bytes.hpp"
#include "valdi_core/cpp/Utils/Result.hpp"
#include "valdi_core/cpp/Utils/StringBox.hpp"
#include "valdi_core/cpp/Utils/Value.hpp"

namespace Valdi {

class IRemoteDownloaderItemHandler {
public:
    virtual ~IRemoteDownloaderItemHandler() = default;

    /**
     Preprocess data which was downloaded remotely. The returned data will be stored in
     the disk cache and be provided to the transform function.
     */
    virtual Result<BytesView> preprocess(const StringBox& /*localFilename*/, const BytesView& remoteData) const {
        return remoteData;
    }

    /**
     Transform the data into a higher level representation which will be stored in memory
     and provided to the completion callback.
     */
    virtual Result<Value> transform(const StringBox& localFilename, const BytesView& data) const = 0;

    /**
     Returns a string describing the type of items that the transformer handles
     */
    virtual std::string_view getItemTypeDescription() const = 0;
};

} // namespace Valdi
