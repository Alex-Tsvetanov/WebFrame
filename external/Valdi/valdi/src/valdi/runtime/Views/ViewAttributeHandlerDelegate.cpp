//
//  ViewAttributeHandlerDelegate.cpp
//  valdi
//
//  Created by Simon Corsin on 7/12/22.
//

#include "valdi/runtime/Views/ViewAttributeHandlerDelegate.hpp"
#include "valdi/runtime/Context/ViewNode.hpp"
#include "valdi/runtime/Interfaces/IViewTransaction.hpp"
#include "valdi/runtime/Views/ViewTransactionScope.hpp"

namespace Valdi {

Result<Void> ViewAttributeHandlerDelegate::onApply(ViewTransactionScope& viewTransactionScope,
                                                   ViewNode& viewNode,
                                                   const Ref<View>& view,
                                                   const StringBox& name,
                                                   const Value& value,
                                                   const Ref<Animator>& animator) {
    viewTransactionScope.transaction().executeInTransactionThread(
        [view, animator, self = strongSmallRef(this), viewNode = strongSmallRef(&viewNode), value, name]() {
            auto result = self->onViewApply(view, name, value, animator);
            if (!result) {
                viewNode->notifyAttributeFailed(viewNode->getAttributeIds().getIdForName(name), result.error());
            }
        });

    return Void();
}

void ViewAttributeHandlerDelegate::onReset(ViewTransactionScope& viewTransactionScope,
                                           ViewNode& viewNode,
                                           const Ref<View>& view,
                                           const StringBox& name,
                                           const Ref<Animator>& animator) {
    viewTransactionScope.transaction().executeInTransactionThread(
        [view, animator, self = strongSmallRef(this), name]() { self->onViewReset(view, name, animator); });
}

} // namespace Valdi
