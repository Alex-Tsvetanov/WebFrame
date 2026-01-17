//
//  AttributeHandlerDelegateWithCallable.hpp
//  valdi-ios
//
//  Created by Simon Corsin on 8/2/21.
//

#pragma once

#include "valdi/runtime/Attributes/AttributeHandlerDelegate.hpp"

namespace Valdi {

class AttributeHandlerDelegateWithCallable : public AttributeHandlerDelegate {
public:
    using Applier = Result<Void> (*)(ViewTransactionScope&, ViewNode&, const Value&);
    using Resetter = void (*)(ViewTransactionScope&, ViewNode&);

    AttributeHandlerDelegateWithCallable(Applier applier, Resetter resetter);

    Result<Void> onApply(ViewTransactionScope& viewTransactionScope,
                         ViewNode& viewNode,
                         const Ref<View>& view,
                         const StringBox& name,
                         const Value& value,
                         const Ref<Animator>& animator) override;
    void onReset(ViewTransactionScope& viewTransactionScope,
                 ViewNode& viewNode,
                 const Ref<View>& view,
                 const StringBox& name,
                 const Ref<Animator>& animator) override;

private:
    Applier _applier;
    Resetter _resetter;
};

} // namespace Valdi
