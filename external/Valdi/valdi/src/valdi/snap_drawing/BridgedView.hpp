//
//  BridgedView.hpp
//  valdi-android
//
//  Created by Simon Corsin on 2/15/22.
//

#pragma once

#include "snap_drawing/cpp/Drawing/Surface/ExternalSurface.hpp"
#include "valdi/runtime/Views/View.hpp"

namespace Valdi {
class IViewManager;
class IViewTransaction;
class ViewNodeTree;
} // namespace Valdi

namespace snap::drawing {

class BridgedView : public ExternalSurface {
public:
    BridgedView(const Valdi::Ref<Valdi::View>& view, Valdi::IViewManager& viewManager);
    ~BridgedView() override;

    const Valdi::Ref<Valdi::View>& getView() const;
    Valdi::IViewManager& getViewManager() const;

    Valdi::Ref<Valdi::IBitmapFactory> getRasterBitmapFactory() const override;

    Valdi::Result<Valdi::Void> rasterInto(const Valdi::Ref<Valdi::IBitmap>& bitmap,
                                          const Rect& frame,
                                          const Matrix& transform,
                                          float rasterScaleX,
                                          float rasterScaleY) override;

    Valdi::IViewTransaction& getViewTransaction(Valdi::ViewNodeTree* viewNodeTree) const;

private:
    Valdi::Ref<Valdi::View> _view;
    Valdi::IViewManager& _viewManager;
};

} // namespace snap::drawing
