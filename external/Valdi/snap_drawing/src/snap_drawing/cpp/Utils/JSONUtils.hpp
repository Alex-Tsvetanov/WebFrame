//
//  JSONUtils.hpp
//  snap_drawing
//
//  Created by Simon Corsin on 3/10/22.
//

#pragma once

#include "snap_drawing/cpp/Utils/Geometry.hpp"
#include "valdi_core/cpp/Utils/Value.hpp"

namespace snap::drawing {

Valdi::Value toJSONValue(const Rect& rect);
Valdi::Value toJSONValue(const Size& size);

} // namespace snap::drawing
