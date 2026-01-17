//
//  PlaceholderViewMeasureDelegate.cpp
//  valdi
//
//  Created by Simon Corsin on 6/13/22.
//

#include "valdi/runtime/Views/PlaceholderViewMeasureDelegate.hpp"
#include "valdi/runtime/Context/ViewNode.hpp"
#include "valdi/runtime/Context/ViewNodeTree.hpp"
#include "valdi/runtime/Interfaces/IViewTransaction.hpp"
#include "valdi/runtime/Utils/MainThreadManager.hpp"

namespace Valdi {

PlaceholderViewMeasureDelegate::PlaceholderViewMeasureDelegate() = default;
PlaceholderViewMeasureDelegate::~PlaceholderViewMeasureDelegate() = default;

Size PlaceholderViewMeasureDelegate::measure(
    ViewNode& viewNode, float width, MeasureMode widthMode, float height, MeasureMode heightMode) {
    auto* viewNodeTree = viewNode.getViewNodeTree();
    auto currentViewTransactionScope = viewNodeTree->getCurrentViewTransactionScopeRef();
    auto& transaction = currentViewTransactionScope->transaction();

    currentViewTransactionScope->flushNow(/* sync */ true);

    Size measuredSize;
    transaction.executeInTransactionThread([&]() {
        // We set a nested direct view transaction scope during the measure pass, so that all operations that
        // occur are direct.
        auto nestedViewTransactionScope = makeShared<ViewTransactionScope>(
            viewNodeTree->getViewManager(), currentViewTransactionScope->getMainThreadManager(), false);
        viewNodeTree->unsafeSetCurrentViewTransactionScope(nestedViewTransactionScope);

        if (viewNode.getView() != nullptr) {
            measuredSize = measureView(viewNode.getView(), width, widthMode, height, heightMode);
        } else {
            std::lock_guard<Mutex> guard(_mutex);
            if (!_didFetchPlaceholderView) {
                _didFetchPlaceholderView = true;
                _placeholderView = createPlaceholderView();
            }

            if (_placeholderView == nullptr) {
                measuredSize = Size();
                return;
            }

            auto placeholderViewNode = viewNode.makePlaceholderViewNode(*nestedViewTransactionScope, _placeholderView);

            nestedViewTransactionScope->flushNow(/* sync */ true);

            measuredSize = measureView(_placeholderView, width, widthMode, height, heightMode);

            nestedViewTransactionScope->transaction().moveViewToTree(_placeholderView, viewNodeTree, nullptr);
            placeholderViewNode->removeView(*nestedViewTransactionScope);
            nestedViewTransactionScope->submit();
        }
    });

    currentViewTransactionScope->flushNow(/* sync */ true);
    // Set the view transaction back to what it was before measuring
    viewNodeTree->unsafeSetCurrentViewTransactionScope(currentViewTransactionScope);

    return measuredSize;
}

} // namespace Valdi
