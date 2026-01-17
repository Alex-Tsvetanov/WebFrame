//
//  DownloadableModuleManifest.hpp
//  valdi-ios
//
//  Created by Simon Corsin on 11/8/19.
//

#pragma once

#include "valdi/valdi.pb.h"
#include "valdi_core/cpp/Utils/Shared.hpp"

namespace Valdi {

class DownloadableModuleManifestWrapper : public SimpleRefCountable {
public:
    DownloadableModuleManifestWrapper();
    ~DownloadableModuleManifestWrapper() override;

    Valdi::DownloadableModuleManifest pb;
};

} // namespace Valdi
