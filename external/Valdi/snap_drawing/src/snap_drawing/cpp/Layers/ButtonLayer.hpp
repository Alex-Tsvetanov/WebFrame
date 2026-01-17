//
//  ButtonLayer.hpp
//  valdi-skia
//
//  Created by Simon Corsin on 6/28/20.
//

#pragma once

#include "snap_drawing/cpp/Layers/TextLayer.hpp"

namespace snap::drawing {

class ButtonLayer : public TextLayer {
public:
    explicit ButtonLayer(const Ref<Resources>& resources);
};

} // namespace snap::drawing
