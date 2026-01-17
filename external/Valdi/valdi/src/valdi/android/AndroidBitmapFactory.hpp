#pragma once

#include "valdi_core/cpp/Interfaces/IBitmapFactory.hpp"

namespace ValdiAndroid {

class AndroidBitmapFactory : public Valdi::IBitmapFactory {
public:
    AndroidBitmapFactory();
    ~AndroidBitmapFactory() override;

    Valdi::Result<Valdi::Ref<Valdi::IBitmap>> createBitmap(int width, int height) override;

    static const Valdi::Ref<AndroidBitmapFactory>& getSharedInstance();
};

} // namespace ValdiAndroid