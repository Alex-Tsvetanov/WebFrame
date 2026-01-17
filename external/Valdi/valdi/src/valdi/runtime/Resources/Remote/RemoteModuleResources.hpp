//
//  RemoteModuleResources.hpp
//  valdi-ios
//
//  Created by Simon Corsin on 8/29/19.
//

#pragma once

#include "valdi_core/cpp/Utils/FlatMap.hpp"
#include "valdi_core/cpp/Utils/Shared.hpp"
#include "valdi_core/cpp/Utils/StringBox.hpp"
#include "valdi_core/cpp/Utils/ValdiObject.hpp"
#include <optional>

namespace Valdi {

class RemoteModuleResources : public ValdiObject {
public:
    explicit RemoteModuleResources(FlatMap<StringBox, StringBox> resourceCacheUrls);
    RemoteModuleResources();

    std::optional<StringBox> getResourceCacheUrl(const StringBox& resourcePath) const;

    const FlatMap<StringBox, StringBox>& getAllUrls() const;

    bool operator==(const RemoteModuleResources& other) const;
    bool operator!=(const RemoteModuleResources& other) const;

    VALDI_CLASS_HEADER(RemoteModuleResources)

private:
    FlatMap<StringBox, StringBox> _resourceCacheUrls;
};

} // namespace Valdi
