//
//  IResourceLoader.hpp
//  valdi-ios
//
//  Created by Simon Corsin on 9/30/19.
//

#pragma once

#include "valdi_core/cpp/Utils/Bytes.hpp"
#include "valdi_core/cpp/Utils/Result.hpp"
#include "valdi_core/cpp/Utils/StringBox.hpp"
#include "valdi_core/cpp/Utils/Value.hpp"

namespace Valdi {

class IResourceLoader {
public:
    virtual ~IResourceLoader() = default;

    /**
     Load the given Valdi module name and return it as bytes.
     */
    virtual Result<BytesView> loadModuleContent(const StringBox& module) = 0;

    /**
     Resolves the local asset URL for the given asset.
     */
    virtual StringBox resolveLocalAssetURL(const StringBox& moduleName, const StringBox& resourcePath) = 0;
};

} // namespace Valdi
