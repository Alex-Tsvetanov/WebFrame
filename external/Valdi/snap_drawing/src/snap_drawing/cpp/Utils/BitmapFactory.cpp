#include "snap_drawing/cpp/Utils/BitmapFactory.hpp"
#include "snap_drawing/cpp/Utils/Bitmap.hpp"

namespace snap::drawing {

BitmapFactory::BitmapFactory(Valdi::ColorType colorType) : _colorType(colorType) {}
BitmapFactory::~BitmapFactory() = default;

Valdi::Result<Ref<Valdi::IBitmap>> BitmapFactory::createBitmap(int width, int height) {
    auto info = Valdi::BitmapInfo(width,
                                  height,
                                  _colorType,
                                  Valdi::AlphaType::AlphaTypePremul,
                                  width * Valdi::BitmapInfo::bytesPerPixelForColorType(_colorType));
    auto result = Bitmap::make(info);
    if (!result) {
        return result.moveError();
    }
    return Ref<Valdi::IBitmap>(result.moveValue());
}

const Ref<BitmapFactory>& BitmapFactory::getInstance(Valdi::ColorType colorType) {
    switch (colorType) {
        case Valdi::ColorType::ColorTypeUnknown:
            std::abort();
        case Valdi::ColorType::ColorTypeRGBA8888: {
            static auto kInstance = Valdi::makeShared<BitmapFactory>(Valdi::ColorType::ColorTypeRGBA8888);
            return kInstance;
        }
        case Valdi::ColorType::ColorTypeBGRA8888: {
            static auto kInstance = Valdi::makeShared<BitmapFactory>(Valdi::ColorType::ColorTypeBGRA8888);
            return kInstance;
        }
        case Valdi::ColorType::ColorTypeAlpha8: {
            static auto kInstance = Valdi::makeShared<BitmapFactory>(Valdi::ColorType::ColorTypeAlpha8);
            return kInstance;
        }
        case Valdi::ColorType::ColorTypeGray8: {
            static auto kInstance = Valdi::makeShared<BitmapFactory>(Valdi::ColorType::ColorTypeGray8);
            return kInstance;
        }
        case Valdi::ColorType::ColorTypeRGBAF16: {
            static auto kInstance = Valdi::makeShared<BitmapFactory>(Valdi::ColorType::ColorTypeRGBAF16);
            return kInstance;
        }
        case Valdi::ColorType::ColorTypeRGBAF32: {
            static auto kInstance = Valdi::makeShared<BitmapFactory>(Valdi::ColorType::ColorTypeRGBAF32);
            return kInstance;
        }
    }
}

} // namespace snap::drawing