//
//  BitmapWithBuffer.hpp
//  valdi
//
//  Created by Simon Corsin on 08/06/2024.
//  Copyright © 2024 Snap Inc. All rights reserved.
//

#pragma once

#include "valdi_core/cpp/Interfaces/IBitmap.hpp"
#include "valdi_core/cpp/Utils/Bytes.hpp"

namespace Valdi {

class BitmapWithBuffer : public IBitmap {
public:
    BitmapWithBuffer(const Valdi::BytesView& buffer, const Valdi::BitmapInfo& bitmapInfo);
    ~BitmapWithBuffer() override;

    void dispose() override;

    Valdi::BitmapInfo getInfo() const override;

    void* lockBytes() override;

    void unlockBytes() override;

private:
    Valdi::BytesView _buffer;
    Valdi::BitmapInfo _bitmapInfo;
};

} // namespace Valdi
