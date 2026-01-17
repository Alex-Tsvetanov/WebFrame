//
//  IBitmap.hpp
//  valdi-desktop-apple
//
//  Created by Simon Corsin on 7/31/20.
//

#pragma once

#include "valdi_core/cpp/Utils/Shared.hpp"
#include "valdi_core/cpp/Utils/ValdiObject.hpp"

namespace Valdi {

enum ColorType {
    ColorTypeUnknown = 0,
    ColorTypeRGBA8888,
    ColorTypeBGRA8888,
    ColorTypeAlpha8,
    ColorTypeGray8,
    ColorTypeRGBAF16,
    ColorTypeRGBAF32
};

enum AlphaType { AlphaTypeOpaque = 0, AlphaTypePremul, AlphaTypeUnpremul };

struct BitmapInfo {
    int width;
    int height;
    ColorType colorType;
    AlphaType alphaType;
    size_t rowBytes;

    BitmapInfo(int width, int height, ColorType colorType, AlphaType alphaType, size_t rowBytes)
        : width(width), height(height), colorType(colorType), alphaType(alphaType), rowBytes(rowBytes) {}

    bool operator==(const BitmapInfo& other) const {
        return (this->width == other.width) && (this->height == other.height) && (this->colorType == other.colorType) &&
               (this->alphaType == other.alphaType) && (this->rowBytes == other.rowBytes);
    }

    bool operator!=(const BitmapInfo& other) const {
        return !(*this == other);
    }

    constexpr size_t bytesLength() const {
        return height * rowBytes;
    }

    static size_t bytesPerPixelForColorType(ColorType colorType);
};

class IBitmap : public ValdiObject {
public:
    virtual void dispose() = 0;

    virtual BitmapInfo getInfo() const = 0;
    virtual void* lockBytes() = 0;
    virtual void unlockBytes() = 0;

    VALDI_CLASS_HEADER(IBitmap)
};

} // namespace Valdi
