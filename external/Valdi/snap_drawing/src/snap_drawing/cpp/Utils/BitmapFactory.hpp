#pragma once

#include "snap_drawing/cpp/Utils/Aliases.hpp"
#include "valdi_core/cpp/Interfaces/IBitmapFactory.hpp"

namespace snap::drawing {

class BitmapFactory : public Valdi::IBitmapFactory {
public:
    explicit BitmapFactory(Valdi::ColorType colorType);
    ~BitmapFactory() override;

    Valdi::Result<Ref<Valdi::IBitmap>> createBitmap(int width, int height) override;

    static const Ref<BitmapFactory>& getInstance(Valdi::ColorType colorType);

private:
    Valdi::ColorType _colorType;
};

} // namespace snap::drawing