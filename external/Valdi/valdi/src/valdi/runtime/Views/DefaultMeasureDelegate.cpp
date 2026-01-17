//
//  DefaultMeasureDelegate.cpp
//  valdi
//
//  Created by Simon Corsin on 6/10/22.
//

#include "valdi/runtime/Views/DefaultMeasureDelegate.hpp"
#include "valdi/runtime/Context/ViewNode.hpp"
#include "valdi/runtime/Views/View.hpp"

namespace Valdi {

DefaultMeasureDelegate::DefaultMeasureDelegate() = default;
DefaultMeasureDelegate::~DefaultMeasureDelegate() = default;

Size DefaultMeasureDelegate::measure(
    ViewNode& viewNode, float width, MeasureMode widthMode, float height, MeasureMode heightMode) {
    auto layoutAttributes = viewNode.copyProcessedViewLayoutAttributes();
    if (!layoutAttributes) {
        return Valdi::Size();
    }

    return onMeasure(layoutAttributes.value(), width, widthMode, height, heightMode, viewNode.isRightToLeft());
}

} // namespace Valdi
