//
//  BitmapWithBuffer.cpp
//  valdi
//
//  Created by Simon Corsin on 08/06/2024.
//  Copyright © 2024 Snap Inc. All rights reserved.
//

#include "valdi_core/cpp/Utils/BitmapWithBuffer.hpp"

namespace Valdi {

BitmapWithBuffer::BitmapWithBuffer(const Valdi::BytesView& buffer, const Valdi::BitmapInfo& bitmapInfo)
    : _buffer(buffer), _bitmapInfo(bitmapInfo) {}
BitmapWithBuffer::~BitmapWithBuffer() = default;

void BitmapWithBuffer::dispose() {
    _buffer = Valdi::BytesView();
}

Valdi::BitmapInfo BitmapWithBuffer::getInfo() const {
    return _bitmapInfo;
}

void* BitmapWithBuffer::lockBytes() {
    return const_cast<Valdi::Byte*>(_buffer.data());
}

void BitmapWithBuffer::unlockBytes() {}

} // namespace Valdi
