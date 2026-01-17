//
//  BridgedView.cpp
//  valdi-android
//
//  Created by Simon Corsin on 2/15/22.
//

#include "BridgedView.hpp"
#include "snap_drawing/cpp/Drawing/Surface/ExternalSurfacePresenterState.hpp"
#include "snap_drawing/cpp/Utils/Bitmap.hpp"
#include "snap_drawing/cpp/Utils/Image.hpp"
#include "valdi/runtime/Context/ViewNodeTree.hpp"
#include "valdi/runtime/Interfaces/IViewManager.hpp"
#include "valdi_core/cpp/Interfaces/IBitmap.hpp"
#include "valdi_core/cpp/Interfaces/IBitmapFactory.hpp"

namespace snap::drawing {

BridgedView::BridgedView(const Valdi::Ref<Valdi::View>& view, Valdi::IViewManager& viewManager)
    : _view(view), _viewManager(viewManager) {}

BridgedView::~BridgedView() = default;

const Valdi::Ref<Valdi::View>& BridgedView::getView() const {
    return _view;
}

Valdi::IViewManager& BridgedView::getViewManager() const {
    return _viewManager;
}

Valdi::Ref<Valdi::IBitmapFactory> BridgedView::getRasterBitmapFactory() const {
    return _viewManager.getViewRasterBitmapFactory();
}

Valdi::Result<Valdi::Void> BridgedView::rasterInto(const Valdi::Ref<Valdi::IBitmap>& bitmap,
                                                   const Rect& frame,
                                                   const Matrix& transform,
                                                   float rasterScaleX,
                                                   float rasterScaleY) {
    if (_view == nullptr) {
        return Valdi::Error("Not view attached");
    }

    auto valdiFrame = Valdi::Frame(frame.x(), frame.y(), frame.width(), frame.height());
    Valdi::Matrix valdiTransform;
    transform.toAffine(&valdiTransform.values[0]);
    return _view->rasterInto(bitmap, valdiFrame, valdiTransform, rasterScaleX, rasterScaleY);
}

Valdi::IViewTransaction& BridgedView::getViewTransaction(Valdi::ViewNodeTree* viewNodeTree) const {
    return viewNodeTree->getCurrentViewTransactionScope().withViewManager(_viewManager).transaction();
}

} // namespace snap::drawing
