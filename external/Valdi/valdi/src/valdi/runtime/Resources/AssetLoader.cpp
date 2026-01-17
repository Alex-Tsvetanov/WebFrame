//
//  AssetLoader.cpp
//  valdi-ios
//
//  Created by Simon Corsin on 8/12/21.
//

#include "valdi/runtime/Resources/AssetLoader.hpp"

namespace Valdi {

AssetLoader::AssetLoader(std::vector<StringBox> supportedSchemes) : _supportedSchemes(std::move(supportedSchemes)) {}

AssetLoader::~AssetLoader() = default;

} // namespace Valdi
