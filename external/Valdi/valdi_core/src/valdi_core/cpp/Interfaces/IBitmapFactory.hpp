#pragma once

#include "valdi_core/cpp/Interfaces/IBitmap.hpp"
#include "valdi_core/cpp/Utils/Result.hpp"

namespace Valdi {

class IBitmapFactory : public SimpleRefCountable {
public:
    virtual Result<Ref<IBitmap>> createBitmap(int width, int height) = 0;
};

} // namespace Valdi