//
//  MeasureDelegate.hpp
//  valdi
//
//  Created by Simon Corsin on 7/10/22.
//

#pragma once

#include "valdi_core/cpp/Utils/Shared.hpp"

#include "valdi/runtime/Views/Frame.hpp"
#include "valdi/runtime/Views/Measure.hpp"

namespace Valdi {

class ViewNode;

class MeasureDelegate : public SimpleRefCountable {
public:
    virtual Size measure(
        ViewNode& viewNode, float width, MeasureMode widthMode, float height, MeasureMode heightMode) = 0;
};

} // namespace Valdi
