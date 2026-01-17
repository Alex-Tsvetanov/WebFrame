//
//  StandaloneLoadedAsset.hpp
//  valdi-pc
//
//  Created by Simon Corsin on 10/14/19.
//

#pragma once

#include "valdi_core/cpp/Resources/LoadedAsset.hpp"
#include "valdi_core/cpp/Utils/Bytes.hpp"

namespace Valdi {

class StandaloneLoadedAsset : public LoadedAsset {
public:
    StandaloneLoadedAsset(const BytesView& bytes, int width, int height);
    ~StandaloneLoadedAsset() override;

    Result<BytesView> getBytesContent() override;

    const BytesView& getBytes() const;

    constexpr int getWidth() const {
        return _width;
    }

    constexpr int getHeight() const {
        return _height;
    }

    VALDI_CLASS_HEADER(StandaloneLoadedAsset)

private:
    BytesView _bytes;
    int _width = 0;
    int _height = 0;
};

} // namespace Valdi
