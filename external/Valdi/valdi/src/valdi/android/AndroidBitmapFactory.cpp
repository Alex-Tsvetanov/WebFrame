#include "AndroidBitmapFactory.hpp"
#include "AndroidBitmap.hpp"

namespace ValdiAndroid {

AndroidBitmapFactory::AndroidBitmapFactory() {}
AndroidBitmapFactory::~AndroidBitmapFactory() {}

Valdi::Result<Valdi::Ref<Valdi::IBitmap>> AndroidBitmapFactory::createBitmap(int width, int height) {
    auto info = Valdi::BitmapInfo(
        width, height, Valdi::ColorType::ColorTypeBGRA8888, Valdi::AlphaType::AlphaTypePremul, width * 4);
    auto bitmap = AndroidBitmap::make(info);
    if (!bitmap) {
        return bitmap.moveError();
    }

    return Valdi::Result<Valdi::Ref<Valdi::IBitmap>>(bitmap.moveValue());
}

const Valdi::Ref<AndroidBitmapFactory>& AndroidBitmapFactory::getSharedInstance() {
    static auto kInstance = Valdi::makeShared<AndroidBitmapFactory>();
    return kInstance;
}

} // namespace ValdiAndroid