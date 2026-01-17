//
//  StandaloneLoadedAsset.cpp
//  valdi-pc
//
//  Created by Simon Corsin on 10/14/19.
//

#include "valdi/standalone_runtime/StandaloneLoadedAsset.hpp"

namespace Valdi {

StandaloneLoadedAsset::StandaloneLoadedAsset(const BytesView& bytes, int width, int height)
    : _bytes(bytes), _width(width), _height(height) {}
StandaloneLoadedAsset::~StandaloneLoadedAsset() = default;

Result<BytesView> StandaloneLoadedAsset::getBytesContent() {
    return getBytes();
}

const BytesView& StandaloneLoadedAsset::getBytes() const {
    return _bytes;
}

VALDI_CLASS_IMPL(StandaloneLoadedAsset)

} // namespace Valdi
