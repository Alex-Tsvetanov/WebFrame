//
//  AttributeHandlerDelegateWithCallable.cpp
//  valdi-ios
//
//  Created by Simon Corsin on 8/2/21.
//

#include "valdi/runtime/Attributes/AttributeHandlerDelegateWithCallable.hpp"

namespace Valdi {

AttributeHandlerDelegateWithCallable::AttributeHandlerDelegateWithCallable(Applier applier, Resetter resetter)
    : _applier(applier), _resetter(resetter) {}

Result<Void> AttributeHandlerDelegateWithCallable::onApply(ViewTransactionScope& viewTransactionScope,
                                                           ViewNode& viewNode,
                                                           const Ref<View>& /*view*/,
                                                           const StringBox& /*name*/,
                                                           const Value& value,
                                                           const Ref<Animator>& /*animator*/) {
    return _applier(viewTransactionScope, viewNode, value);
}

void AttributeHandlerDelegateWithCallable::onReset(ViewTransactionScope& viewTransactionScope,
                                                   ViewNode& viewNode,
                                                   const Ref<View>& /*view*/,
                                                   const StringBox& /*name*/,
                                                   const Ref<Animator>& /*animator*/) {
    _resetter(viewTransactionScope, viewNode);
}

} // namespace Valdi
