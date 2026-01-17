//
//  BytesAsset.hpp
//  valdi
//
//  Created by Simon Corsin on 7/19/22.
//

#pragma once

#include "valdi_core/cpp/Resources/LoadedAsset.hpp"
#include "valdi_core/cpp/Utils/Bytes.hpp"

namespace Valdi {

class BytesAsset : public LoadedAsset {
public:
    explicit BytesAsset(const BytesView& bytes);
    ~BytesAsset() override;

    Result<BytesView> getBytesContent() override;

    VALDI_CLASS_HEADER(BytesAsset)

private:
    BytesView _bytes;
};

} // namespace Valdi
