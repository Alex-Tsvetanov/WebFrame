//
//  ButtonLayer.cpp
//  valdi-skia
//
//  Created by Simon Corsin on 6/28/20.
//

#include "snap_drawing/cpp/Layers/ButtonLayer.hpp"

namespace snap::drawing {

ButtonLayer::ButtonLayer(const Ref<Resources>& resources) : TextLayer(resources) {
    setTextAlign(TextAlignCenter);
}

} // namespace snap::drawing
