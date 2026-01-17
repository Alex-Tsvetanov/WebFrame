#pragma once

#include "snap_drawing/cpp/Utils/Image.hpp"
#include <utility>

namespace snap::imagetoolbox {

class SVGRenderer {
public:
    static Valdi::Result<std::pair<int, int>> getSize(const Valdi::Byte* data, size_t length);

    static Valdi::Result<snap::drawing::Ref<snap::drawing::Image>> render(const Valdi::Byte* data,
                                                                          size_t length,
                                                                          int outputWidth,
                                                                          int outputHeight);
};

} // namespace snap::imagetoolbox
