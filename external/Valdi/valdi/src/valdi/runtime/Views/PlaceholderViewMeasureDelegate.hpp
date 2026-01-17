//
//  PlaceholderViewMeasureDelegate.hpp
//  valdi
//
//  Created by Simon Corsin on 6/13/22.
//

#pragma once

#include "valdi/runtime/Views/MeasureDelegate.hpp"
#include "valdi/runtime/Views/View.hpp"
#include "valdi_core/cpp/Utils/Mutex.hpp"

namespace Valdi {

class PlaceholderViewMeasureDelegate : public MeasureDelegate {
public:
    PlaceholderViewMeasureDelegate();
    ~PlaceholderViewMeasureDelegate() override;

    Size measure(ViewNode& viewNode, float width, MeasureMode widthMode, float height, MeasureMode heightMode) override;

    virtual Size measureView(
        const Ref<View>& view, float width, MeasureMode widthMode, float height, MeasureMode heightMode) = 0;

    virtual Ref<View> createPlaceholderView() = 0;

private:
    Mutex _mutex;
    Ref<View> _placeholderView;
    bool _didFetchPlaceholderView = false;
};

} // namespace Valdi
