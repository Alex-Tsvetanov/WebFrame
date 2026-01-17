//
//  Bitmap.hpp
//  snap_drawing
//
//  Created by Ramzy Jaber on 3/28/22.
//

#pragma once

#include "valdi_core/cpp/Interfaces/IBitmap.hpp"
#include "valdi_core/cpp/Utils/Result.hpp"

#include "include/core/SkBitmap.h"

#include "snap_drawing/cpp/Utils/Aliases.hpp"

namespace snap::drawing {

class Bitmap : public Valdi::IBitmap {
public:
    explicit Bitmap(SkBitmap&& bitmap);

    ~Bitmap() override = default;

    void dispose() override;

    Valdi::BitmapInfo getInfo() const override;

    void* lockBytes() override;

    void unlockBytes() override;

    static Valdi::Result<Ref<Bitmap>> make(const Valdi::BitmapInfo& info);

private:
    SkBitmap _bitmap;
};

} // namespace snap::drawing
