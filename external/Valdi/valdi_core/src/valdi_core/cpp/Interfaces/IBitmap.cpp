//
//  IBitmap.cpp
//  valdi-desktop-apple
//
//  Created by Simon Corsin on 7/18/24.
//

#include "valdi_core/cpp/Interfaces/IBitmap.hpp"

namespace Valdi {

VALDI_CLASS_IMPL(IBitmap)

size_t BitmapInfo::bytesPerPixelForColorType(ColorType colorType) {
    switch (colorType) {
        case Valdi::ColorType::ColorTypeUnknown:
            return 0;
        case Valdi::ColorType::ColorTypeRGBA8888:
            return 4;
        case Valdi::ColorType::ColorTypeBGRA8888:
            return 4;
        case Valdi::ColorType::ColorTypeAlpha8:
            return 1;
        case Valdi::ColorType::ColorTypeGray8:
            return 1;
        case Valdi::ColorType::ColorTypeRGBAF16:
            return 8;
        case Valdi::ColorType::ColorTypeRGBAF32:
            return 16;
    }
}

} // namespace Valdi
