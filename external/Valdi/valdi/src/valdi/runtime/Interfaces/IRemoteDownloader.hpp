//
//  IDownloader.h
//  valdi
//
//  Created by Ramzy Jaber on 3/11/22.
//

#pragma once

#include "valdi_core/Cancelable.hpp"
#include "valdi_core/cpp/Utils/Bytes.hpp"
#include "valdi_core/cpp/Utils/Function.hpp"
#include "valdi_core/cpp/Utils/Result.hpp"
#include "valdi_core/cpp/Utils/Shared.hpp"
#include "valdi_core/cpp/Utils/StringBox.hpp"

namespace Valdi {

class IRemoteDownloader : public SharedPtrRefCountable {
public:
    virtual Shared<snap::valdi_core::Cancelable> downloadItem(const StringBox& url,
                                                              Function<void(const Result<BytesView>&)> completion) = 0;
};

} // namespace Valdi
