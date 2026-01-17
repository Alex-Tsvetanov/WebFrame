//
//  BytesAsset.cpp
//  valdi
//
//  Created by Simon Corsin on 7/19/22.
//

#include "valdi/runtime/Resources/BytesAsset.hpp"

namespace Valdi {

BytesAsset::BytesAsset(const BytesView& bytes) : _bytes(bytes) {}
BytesAsset::~BytesAsset() = default;

Result<BytesView> BytesAsset::getBytesContent() {
    return _bytes;
}

VALDI_CLASS_IMPL(BytesAsset)

} // namespace Valdi
